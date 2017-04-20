/**
* Multithreaded, libevent 2.x-based socket server.
* Copyright (c) 2012 Qi Huang
* This software is licensed under the BSD license.
* See the accompanying LICENSE.txt for details.
*
* To compile: ./make
* To run: ./echoserver_threaded
*/

#ifndef _WIN32
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#include <sys/socket.h>
#endif
#pragma comment(lib, "ws2_32.lib")

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
//#include <err.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
//#include <signal.h>
#define DATA_SIZE 237608

//#include "workqueue.h"

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

int(*do_logic_func)(unsigned char* data, void* client_data);
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

/*
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
*/

static void echo_read_cb(struct bufferevent *bev, void *ctx)
{
	unsigned char* data = (unsigned char*)ctx + sizeof(int);
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

	void* data = malloc(DATA_SIZE + sizeof(int));
	memset(data, 0, DATA_SIZE + sizeof(int));
	bufferevent_setcb(bev, echo_read_cb, NULL, echo_event_cb, data);

	bufferevent_enable(bev, EV_READ | EV_WRITE);
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
int runServer(int(*logic_func)(unsigned char* data, void* client_data),
	void* (*check_answer_func)(bool* is_empty, int* answer))
{
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
		LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
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
}