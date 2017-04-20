/**
 * Multithreaded, libevent 2.x-based socket server.
 * Copyright (c) 2012 Qi Huang
 * This software is licensed under the BSD license.
 * See the accompanying LICENSE.txt for details.
 *
 * To compile: ./make
 * To run: ./echoserver_threaded
 */

#include <assert.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
#include <signal.h>
#define USER_MODE

#define DATA_SIZE 237608

#include "../../../../linux-sgx-driver/isgx_user.h"

#include "workqueue.h"

/* Port to listen on. */
#define SERVER_PORT 5006
/* Connection backlog (# of backlogged connections to accept). */
#define CONNECTION_BACKLOG 8
/* Number of worker threads.  Should match number of CPU cores reported in 
 * /proc/cpuinfo. */
#define NUM_THREADS 1

/* Behaves similarly to fprintf(stderr, ...), but adds file, line, and function
 information. */
#define errorOut(...) {\
    fprintf(stderr, "%s:%d: %s():\t", __FILE__, __LINE__, __FUNCTION__);\
    fprintf(stderr, __VA_ARGS__);\
}

int (*do_logic_func)(unsigned char* data, void* client_data);
void* (*do_check_answer_func)(bool* is_empty, int* answer);

/**
 * Struct to carry around connection (client)-specific data.
 */
typedef struct client {
    /* The client's socket. */
    int fd;

    /* The event_base for this client. */
    struct event_base *evbase;

    /* The bufferedevent for this client. */
    struct bufferevent *buf_ev;

    /* The output buffer for this client. */
    struct evbuffer *output_buffer;

    /* Here you can add your own application-specific attributes which
     * are connection-specific. */
} client_t;

static struct event_base *evbase_accept;
static workqueue_t workqueue;

/* Signal handler function (defined below). */
static void sighandler(int signal);

static void closeClient(client_t *client) {
    if (client != NULL) {
        if (client->fd >= 0) {
            close(client->fd);
            client->fd = -1;
        }
    }
}

static void closeAndFreeClient(client_t *client) {
    if (client != NULL) {
        closeClient(client);
        if (client->buf_ev != NULL) {
            bufferevent_free(client->buf_ev);
            client->buf_ev = NULL;
        }
        if (client->evbase != NULL) {
            event_base_free(client->evbase);
            client->evbase = NULL;
        }
        if (client->output_buffer != NULL) {
            evbuffer_free(client->output_buffer);
            client->output_buffer = NULL;
        }
        free(client);
    }
}

/**
 * Called by libevent when there is data to read.
 */
void buffered_on_read(struct bufferevent *bev, void *arg) {
    client_t *client = (client_t *)arg;
    static char data[DATA_SIZE];
    static int total_read = 0;
    int nbytes;

    /* If we have input data, the number of bytes we have is contained in
     * bev->input->off. Copy the data from the input buffer to the output
     * buffer in 4096-byte chunks. There is a one-liner to do the whole thing
     * in one shot, but the purpose of this server is to show actual real-world
     * reading and writing of the input and output buffers, so we won't take
     * that shortcut here. */
    struct evbuffer *input;
    input = bufferevent_get_input(bev);
    while (evbuffer_get_length(input) > 0) {
        /* Remove a chunk of data from the input buffer, copying it into our
         * local array (data). */
        nbytes = evbuffer_remove(input, data + total_read, 4096);
	total_read += nbytes;
	//printf("%d: read %d bytes from user\n", pthread_self(), nbytes);
    }

    if (total_read < DATA_SIZE) return;
    //printf("read %d bytes...sending to enclave\n", total_read);
    total_read = 0; // reset for next message...

    int answer = do_logic_func((unsigned char*)data, bev);
    /* Add the chunk of data from our local array (data) to the client's
    * output buffer. */
    evbuffer_add(client->output_buffer, &answer, sizeof(int));

    /* Send the results to the client.  This actually only queues the results
     * for sending. Sending will occur asynchronously, handled by libevent. */
    if (bufferevent_write_buffer(bev, client->output_buffer)) {
        errorOut("Error sending data to client on fd %d\n", client->fd);
        closeClient(client);
    }
}

/**
 * Called by libevent when the write buffer reaches 0.  We only
 * provide this because libevent expects it, but we don't use it.
 */
void buffered_on_write(struct bufferevent *bev, void *arg) {
}

/**
 * Called by libevent when there is an error on the underlying socket
 * descriptor.
 */
void buffered_on_error(struct bufferevent *bev, short what, void *arg) {
    closeClient((client_t *)arg);
}

static void server_job_function(struct job *job) {
    client_t *client = (client_t *)job->user_data;

    event_base_dispatch(client->evbase);
    closeAndFreeClient(client);
    free(job);
}

/**
 * This function will be called by libevent when there is a connection
 * ready to be accepted.
 */
void on_accept(evutil_socket_t fd, short ev, void *arg) {
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    workqueue_t *workqueue = (workqueue_t *)arg;
    client_t *client;
    job_t *job;

    client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
        warn("accept failed");
        return;
    }

    /* Set the client socket to non-blocking mode. */
    if (evutil_make_socket_nonblocking(client_fd) < 0) {
        warn("failed to set client socket to non-blocking");
        close(client_fd);
        return;
    }

    /* Create a client object. */
    if ((client = (client_t*)malloc(sizeof(*client))) == NULL) {
        warn("failed to allocate memory for client state");
        close(client_fd);
        return;
    }
    memset(client, 0, sizeof(*client));
    client->fd = client_fd;

    /* Add any custom code anywhere from here to the end of this function
     * to initialize your application-specific attributes in the client struct.
     */

    if ((client->output_buffer = evbuffer_new()) == NULL) {
        warn("client output buffer allocation failed");
        closeAndFreeClient(client);
        return;
    }

    if ((client->evbase = event_base_new()) == NULL) {
        warn("client event_base creation failed");
        closeAndFreeClient(client);
        return;
    }

    /* Create the buffered event.
     *
     * The first argument is the file descriptor that will trigger
     * the events, in this case the clients socket.
     *
     * The second argument is the callback that will be called
     * when data has been read from the socket and is available to
     * the application.
     *
     * The third argument is a callback to a function that will be
     * called when the write buffer has reached a low watermark.
     * That usually means that when the write buffer is 0 length,
     * this callback will be called.  It must be defined, but you
     * don't actually have to do anything in this callback.
     *
     * The fourth argument is a callback that will be called when
     * there is a socket error.  This is where you will detect
     * that the client disconnected or other socket errors.
     *
     * The fifth and final argument is to store an argument in
     * that will be passed to the callbacks.  We store the client
     * object here.
     */
    client->buf_ev = bufferevent_socket_new(client->evbase, client_fd,
                                            BEV_OPT_CLOSE_ON_FREE);
    if ((client->buf_ev) == NULL) {
        warn("client bufferevent creation failed");
        closeAndFreeClient(client);
        return;
    }
    bufferevent_setcb(client->buf_ev, buffered_on_read, buffered_on_write,
                      buffered_on_error, client);

    /* We have to enable it before our callbacks will be
     * called. */
    bufferevent_enable(client->buf_ev, EV_READ);

    /* Create a job object and add it to the work queue. */
    if ((job = (job_t*)malloc(sizeof(*job))) == NULL) {
        warn("failed to allocate memory for job state");
        closeAndFreeClient(client);
        return;
    }
    job->job_function = server_job_function;
    job->user_data = client;

    workqueue_add_job(workqueue, job);
}

//#define MSG_SIZE 262184

struct isgx_user_pages_param param;
int sgx_fd;

static void
echo_read_cb(struct bufferevent *bev, void *ctx)
{
    //client_t *client = (client_t *)arg;
    //static char data[262184];
    unsigned char* data = (unsigned char*)ctx+sizeof(int);
    int* total_read = (int*)ctx;
    int nbytes;

    /* If we have input data, the number of bytes we have is contained in
     * bev->input->off. Copy the data from the input buffer to the output
     * buffer in 4096-byte chunks. There is a one-liner to do the whole thing
     * in one shot, but the purpose of this server is to show actual real-world
     * reading and writing of the input and output buffers, so we won't take
     * that shortcut here. */
    struct evbuffer *input;
    input = bufferevent_get_input(bev);
    while (evbuffer_get_length(input) > 0) {
        /* Remove a chunk of data from the input buffer, copying it into our
         * local array (data). */
        nbytes = evbuffer_remove(input, data + *total_read, 4096);
        *total_read += nbytes;
        //printf("%d: read %d bytes from user\n", pthread_self(), nbytes);
    }


    if (*total_read < DATA_SIZE) return;
    //printf("read %d bytes...sending to enclave\n", *total_read);
    *total_read = 0; // reset for next message...

    do_logic_func((unsigned char*)data, bev);
}

static void
echo_event_cb(struct bufferevent *bev, short events, void *ctx)
{
        if (events & BEV_EVENT_ERROR)
                perror("Error from bufferevent");
        if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
		//int err = ioctl(sgx_fd, ISGX_IOCTL_USER_PAGES, &param);
                //assert(!err);
                //int nr_driver_pf1 = param.quota;
		//printf("client disconnected assumption #PF = %d\n", nr_driver_pf1);
                bufferevent_free(bev);
		free(ctx);
        }
}

static void
accept_conn_cb(struct evconnlistener *listener,
    evutil_socket_t fd, struct sockaddr *address, int socklen,
    void *ctx)
{
        /* We got a new connection! Set up a bufferevent for it. */
        struct event_base *base = evconnlistener_get_base(listener);
        struct bufferevent *bev = bufferevent_socket_new(
                base, fd, BEV_OPT_CLOSE_ON_FREE);

	void* data = malloc(DATA_SIZE+sizeof(int));
	memset(data,0, DATA_SIZE+sizeof(int));
        bufferevent_setcb(bev, echo_read_cb, NULL, echo_event_cb, data);

        bufferevent_enable(bev, EV_READ|EV_WRITE);
}

static void
accept_error_cb(struct evconnlistener *listener, void *ctx)
{
        struct event_base *base = evconnlistener_get_base(listener);
        int err = EVUTIL_SOCKET_ERROR();
        fprintf(stderr, "Got an error %d (%s) on the listener. "
                "Shutting down.\n", err, evutil_socket_error_to_string(err));

        event_base_loopexit(base, NULL);
}

void cb_func(evutil_socket_t fd, short what, void *arg)
{
    //struct event *me = arg;

    while (1) {
	bool is_empty;
	int answer = -1;
	struct bufferevent* bev = (struct bufferevent*)do_check_answer_func(&is_empty, &answer);
	if (is_empty)
	{
		break;
	}

	/* Add the chunk of data from our local array (data) to the client's
        * output buffer. */
	struct evbuffer *output = bufferevent_get_output(bev);

	evbuffer_add(output, &answer, sizeof(int));

        /* Send the results to the client.  This actually only queues the results
        * for sending. Sending will occur asynchronously, handled by libevent. */
	if (bufferevent_write_buffer(bev, output)) {
		errorOut("Error sending data to client on fd %d\n", 0);
	}


    }    

    // delete event with
    //event_del(me);
}

/**
 * Run the server.  This function blocks, only returning when the server has 
 * terminated.
 */
int runServer(int (*logic_func)(unsigned char* data, void* client_data),
	      void* (*check_answer_func)(bool* is_empty, int* answer)) 
{

        sgx_fd = open("/dev/isgx", O_RDWR);

        param.set = 4;          // zeroing the pf counter
        //int err1 = ioctl(sgx_fd, ISGX_IOCTL_USER_PAGES, &param);
        //assert(!err1);
        //param.set = 3;          // for just reading the PF counter. Not changing it

    do_logic_func = logic_func;
    do_check_answer_func = check_answer_func;

struct event_base *base;
        struct evconnlistener *listener;
        struct sockaddr_in sin;

        int port = 5006;

        base = event_base_new();
        if (!base) {
                puts("Couldn't open event base");
                return 1;
        }

        /* Clear the sockaddr before using it, in case there are extra
         * platform-specific fields that can mess us up. */
        memset(&sin, 0, sizeof(sin));
        /* This is an INET address */
        sin.sin_family = AF_INET;
        /* Listen on 0.0.0.0 */
        sin.sin_addr.s_addr = htonl(0);
        /* Listen on the given port. */
        sin.sin_port = htons(port);

        listener = evconnlistener_new_bind(base, accept_conn_cb, NULL,
            LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1,
            (struct sockaddr*)&sin, sizeof(sin));
        if (!listener) {
                perror("Couldn't create listener");
                return 1;
        }
        evconnlistener_set_error_cb(listener, accept_error_cb);

	struct timeval one_sec = { 0, 100 };
	struct event *ev;
	ev = event_new(base, -1, EV_PERSIST, cb_func, NULL); //event_self_cbarg());
	event_add(ev, &one_sec);
	
 	printf("Server running.\n");

        event_base_dispatch(base);
        return 0;

    evutil_socket_t listenfd;
    struct sockaddr_in listen_addr;
    struct event *ev_accept;
    int reuseaddr_on;


    /* Set signal handlers */
    sigset_t sigset;
    sigemptyset(&sigset);
/*
    struct sigaction siginfo = {
        .sa_handler = sighandler,
        .sa_mask = sigset,
        .sa_flags = SA_RESTART,
    };
*/
struct sigaction siginfo;
siginfo.sa_handler = sighandler;
siginfo.sa_mask = sigset;
siginfo.sa_flags = SA_RESTART;

    sigaction(SIGINT, &siginfo, NULL);
    sigaction(SIGTERM, &siginfo, NULL);

    /* Create our listening socket. */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        err(1, "listen failed");
    }

    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(SERVER_PORT);
    if (bind(listenfd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) 
        < 0) {
        err(1, "bind failed");
    }
    if (listen(listenfd, CONNECTION_BACKLOG) < 0) {
        err(1, "listen failed");
    }
    reuseaddr_on = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on,
               sizeof(reuseaddr_on));

    /* Set the socket to non-blocking, this is essential in event
     * based programming with libevent. */
    if (evutil_make_socket_nonblocking(listenfd) < 0) {
        err(1, "failed to set server socket to non-blocking");
    }

    if ((evbase_accept = event_base_new()) == NULL) {
        perror("Unable to create socket accept event base");
        close(listenfd);
        return 1;
    }

    /* Initialize work queue. */
    if (workqueue_init(&workqueue, NUM_THREADS)) {
        perror("Failed to create work queue");
        close(listenfd);
        workqueue_shutdown(&workqueue);
        return 1;
    }

    /* We now have a listening socket, we create a read event to
     * be notified when a client connects. */
    ev_accept = event_new(evbase_accept, listenfd, EV_READ|EV_PERSIST,
                          on_accept, (void *)&workqueue);
    event_add(ev_accept, NULL);

    printf("Server running.\n");

    /* Start the event loop. */
    event_base_dispatch(evbase_accept);

    event_base_free(evbase_accept);
    evbase_accept = NULL;

    close(listenfd);

    printf("Server shutdown.\n");

    return 0;
}

/**
 * Kill the server.  This function can be called from another thread to kill
 * the server, causing runServer() to return.
 */
void killServer(void) {
    fprintf(stdout, "Stopping socket listener event loop.\n");
    if (event_base_loopexit(evbase_accept, NULL)) {
        perror("Error shutting down server");
    }
    fprintf(stdout, "Stopping workers.\n");
    workqueue_shutdown(&workqueue);
}

static void sighandler(int signal) {
    fprintf(stdout, "Received signal %d: %s.  Shutting down.\n", signal,
            strsignal(signal));
    killServer();
}
