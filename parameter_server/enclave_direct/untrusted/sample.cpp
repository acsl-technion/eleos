#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <pwd.h>
#include <libgen.h>
#include <stdlib.h>

# define MAX_PATH FILENAME_MAX

#include <sgx_urts.h>
#include "sample.h"
#include "direct_u.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <time.h>

#include <netdb.h>
#include <netinet/in.h>

#include <pthread.h>

#include "../SimpleHashMap.h"

volatile int thread_start = 0;
volatile int start;
volatile int finish;
int fd;

int request_buf_size;
char* request_buf;

int test_ocall;
int reg;
int rpc;
int ocall_freq = 1;
double acc_untrusted = 0;
double counter_untrusted = 0;
double avg_trusted = 0;
double counter_trusted = 0;
double acc=0;
int newsockfd;
int sockfd;
HashMap hashmap;
unsigned char* temp;
struct timespec ct1,ct2,ce1,ce2;

void* sgx_counting_thread(void *arg)
{
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(2, &cpuset);
	pthread_t thread;
	thread = pthread_self();
	int s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
	if (s != 0)
			printf("pthread_setaffinity_np\n");
	 thread_start = 1;


	 while (true)
	 {
		 if (start == 1)
		 {
			 start = 0;
			 ocall_llc_test(false); 
			 asm volatile (""); // acts as a memory barrier.
			 finish =0;
		 }
		 else if (start == 2)
		 {
			 start = 0;
                         ocall_llc_test(true);
                         asm volatile (""); // acts as a memory barrier.
                         finish =0;
		 }
		 else if (start == 3)
		 {
			 start = 0;
			 clock_gettime(CLOCK_REALTIME, &ct2);
			 double latency = ( ct2.tv_sec - ct1.tv_sec ) + ( ct2.tv_nsec - ct1.tv_nsec ) / 1e9;
			 avg_trusted += latency;
 			 counter_trusted++;
			 asm volatile (""); // acts as a memory barrier.
			 finish =0;
		 }
		 else if (start == 4)
		 {
			 start = 0;
			 clock_gettime(CLOCK_REALTIME, &ce1);
			 asm volatile (""); // acts as a memory barrier.
			 finish =0;
		 }
		 else if (start ==5)
		 {
			 start = 0;
			 clock_gettime(CLOCK_REALTIME, &ce2);
			 double latency = ( ce2.tv_sec - ce1.tv_sec ) + ( ce2.tv_nsec - ce1.tv_nsec ) / 1e9;
			 acc_untrusted += latency;
			 counter_untrusted++;
			 asm volatile (""); // acts as a memory barrier.
			 finish =0;
		 }
		 else
		 {
			 __asm__("pause");
		 }
	 }

	 return NULL;
}


void init_sgx_thread()
{
//	if (rpc)
	{
		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(0, &cpuset);
		pthread_t thread;
		thread = pthread_self();
		int s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
		if (s != 0)
				printf("pthread_setaffinity_np\n");

		pthread_t thread_var;
		pthread_create(&thread_var, NULL, sgx_counting_thread, NULL);

		while (thread_start != 1)
		{
		}
	}
}

void rcall_init_hashmap()
{
	for (size_t i=0;i<HASHMAPSIZE;i++)
	{
		item_t item;
		item.key = i;
		item.val = 0;
		hashmap.add(item);
	}
}

int test_llc_internal(size_t times, int warmup)
{
	volatile long long res = 0;
	int res1;
	item_t* curr;
	long long* request_keys = (long long*)request_buf;
	int request_num = request_buf_size / sizeof(long long);
	for (size_t x=0;x<times;x++)
	{
                if (x % ocall_freq == 0)
                {
                        res1 = ocall_llc_test(warmup);
                        res += res1;
                }


		//for (int i=0;i<HASHMAPSIZE;i++)
		for (int i=0;i<request_num;i++)
		{
			long long r = request_keys[i];
			curr = hashmap.get(r);
			curr->val++;
		}
	}

	return res;
}

int rcall_test_llc_misses_warmup()
{
	return test_llc_internal(10000,true);
}

int rcall_test_llc_misses_real()
{
	return test_llc_internal(AVERAGE_TIMES,false);
}

/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

typedef struct _sgx_errlist_t {
    sgx_status_t err;
    const char *msg;
    const char *sug; /* Suggestion */
} sgx_errlist_t;

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
    {
        SGX_ERROR_UNEXPECTED,
        "Unexpected error occurred.",
        NULL
    },
    {
        SGX_ERROR_INVALID_PARAMETER,
        "Invalid parameter.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_MEMORY,
        "Out of memory.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_LOST,
        "Power transition occurred.",
        "Please refer to the sample \"PowerTransition\" for details."
    },
    {
        SGX_ERROR_INVALID_ENCLAVE,
        "Invalid enclave image.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ENCLAVE_ID,
        "Invalid enclave identification.",
        NULL
    },
    {
        SGX_ERROR_INVALID_SIGNATURE,
        "Invalid enclave signature.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_EPC,
        "Out of EPC memory.",
        NULL
    },
    {
        SGX_ERROR_NO_DEVICE,
        "Invalid SGX device.",
        "Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards."
    },
    {
        SGX_ERROR_MEMORY_MAP_CONFLICT,
        "Memory map conflicted.",
        NULL
    },
    {
        SGX_ERROR_INVALID_METADATA,
        "Invalid enclave metadata.",
        NULL
    },
    {
        SGX_ERROR_DEVICE_BUSY,
        "SGX device was busy.",
        NULL
    },
    {
        SGX_ERROR_INVALID_VERSION,
        "Enclave version was invalid.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ATTRIBUTE,
        "Enclave was not authorized.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_FILE_ACCESS,
        "Can't open enclave file.",
        NULL
    },
};

/* Check error conditions for loading enclave */
void print_error_message(sgx_status_t ret)
{
    size_t idx = 0;
    size_t ttl = sizeof sgx_errlist/sizeof sgx_errlist[0];

    for (idx = 0; idx < ttl; idx++) {
        if(ret == sgx_errlist[idx].err) {
            if(NULL != sgx_errlist[idx].sug)
                printf("Info: %s\n", sgx_errlist[idx].sug);
            printf("Error: %s\n", sgx_errlist[idx].msg);
            break;
        }
    }
    
    if (idx == ttl)
        printf("Error: Unexpected error occurred.\n");
}

/* Initialize the enclave:
 *   Step 1: retrive the launch token saved by last transaction
 *   Step 2: call sgx_create_enclave to initialize an enclave instance
 *   Step 3: save the launch token if it is updated
 */
int initialize_enclave(void)
{
    char token_path[MAX_PATH] = {'\0'};
    sgx_launch_token_t token = {0};
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    int updated = 0;
    /* Step 1: retrive the launch token saved by last transaction */

    /* try to get the token saved in $HOME */
    const char *home_dir = getpwuid(getuid())->pw_dir;
    if (home_dir != NULL && 
        (strlen(home_dir)+strlen("/")+sizeof(TOKEN_FILENAME)+1) <= MAX_PATH) {
        /* compose the token path */
        strncpy(token_path, home_dir, strlen(home_dir));
        strncat(token_path, "/", strlen("/"));
        strncat(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME)+1);
    } else {
        /* if token path is too long or $HOME is NULL */
        strncpy(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME));
    }

    FILE *fp = fopen(token_path, "rb");
    if (fp == NULL && (fp = fopen(token_path, "wb")) == NULL) {
        printf("Warning: Failed to create/open the launch token file \"%s\".\n", token_path);
    }
//    printf("token_path: %s\n", token_path);
    if (fp != NULL) {
        /* read the token from saved file */
        size_t read_num = fread(token, 1, sizeof(sgx_launch_token_t), fp);
        if (read_num != 0 && read_num != sizeof(sgx_launch_token_t)) {
            /* if token is invalid, clear the buffer */
            memset(&token, 0x0, sizeof(sgx_launch_token_t));
            printf("Warning: Invalid launch token read from \"%s\".\n", token_path);
        }
    }

    /* Step 2: call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */

    ret = sgx_create_enclave(DIRECT_FILENAME, SGX_DEBUG_FLAG, &token, &updated, &global_eid, NULL);

    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        if (fp != NULL) fclose(fp);

        return -1;
    }

    /* Step 3: save the launch token if it is updated */

    if (updated == FALSE || fp == NULL) {
        /* if the token is not updated, or file handler is invalid, do not perform saving */
        if (fp != NULL) fclose(fp);
        return 0;
    }

    /* reopen the file with write capablity */
    fp = freopen(token_path, "wb", fp);
    if (fp == NULL) return 0;
    size_t write_num = fwrite(token, 1, sizeof(sgx_launch_token_t), fp);
    if (write_num != sizeof(sgx_launch_token_t))
        printf("Warning: Failed to save launch token to \"%s\".\n", token_path);
    fclose(fp);

    return 0;
}

/* OCall functions */
void ocall_direct_sample(const char *str)
{
    /* Proxy/Bridge will check the length and null-terminate 
     * the input string to prevent buffer overflow. 
     */
    printf("%s", str);
}

void ocall_get_requests()
{

}

int server_process(int request_size)
{
   socklen_t clilen;
   int portno;
   char buffer[256];
   struct sockaddr_in serv_addr, cli_addr;
   int  n;

   /* First call to socket() function */
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   int option = 1;
   setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
   if (sockfd < 0) {
	  perror("ERROR opening socket");
	  exit(1);
   }

   /* Initialize socket structure */
   bzero((char *) &serv_addr, sizeof(serv_addr));
   portno = 5002;

   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(portno);

   /* Now bind the host address using bind() call.*/
   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
	  perror("ERROR on binding");
	  exit(1);
   }

   /* Now start listening for the clients, here process will
	  * go in sleep mode and will wait for the incoming connection
   */

   listen(sockfd,5);
   clilen = sizeof(cli_addr);

   /* Accept actual connection from the client */
   newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

   if (newsockfd < 0) {
	  perror("ERROR on accept");
	  exit(1);
   }

//	   bzero(buffer,request_size);
//	   n = read( newsockfd,buffer,request_size);
//
//	   if (n < 0) {
//		  perror("ERROR reading from socket");
//		  exit(1);
//	   }

   return 0;
}

/* Application entry */
int SGX_CDECL main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    /* Changing dir to where the executable is.*/
    char absolutePath [MAX_PATH];
    char *ptr = NULL;

    ptr = realpath(dirname(argv[0]),absolutePath);

    if( chdir(absolutePath) != 0)
    		abort();

    if (argc < 3)
    {
    	printf("Usage: Reg BUF_SIZE BATCH_SIZE COUNT CACHE_SIZE\n");
    	return -1;
    }

    server_process(0);

    reg = atoi(argv[1]);
    request_buf_size = atoi(argv[2]);
    rpc = atoi(argv[3]);
    int decrpyt_requests = atoi(argv[4]);

//    int num_of_requests = atoi(argv[3]);
//    int count = atoi(argv[4]);
//    int cache_size = atoi(argv[5]);

    /* Initialize the enclave */
    if (!reg)
    {
		if(initialize_enclave() < 0){

			return -1;
		}
    }
 
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    int ecall_return = 0;
    int retval = 0;
    struct timespec c1,c2;

	request_buf = new char[request_buf_size];
	bzero(request_buf, request_buf_size);

    init_sgx_thread();


    temp = (unsigned char*)malloc(request_buf_size);
    if (reg)
    {
    	rcall_init_hashmap();
		rcall_test_llc_misses_warmup();
	}
    else
    {
		ecall_init_hashmap(global_eid, (int*)&start, (int*)&finish,rpc,ocall_freq, request_buf, request_buf_size, temp, decrpyt_requests);
		ecall_test_llc_misses_warmup(global_eid,&retval);
    }

    sleep(1);

    clock_gettime(CLOCK_MONOTONIC, &c1);

    if (reg)
    {
    	rcall_test_llc_misses_real();
    }
    else
    {
    	ecall_test_llc_misses_real(global_eid,&retval);
    }

    clock_gettime(CLOCK_MONOTONIC, &c2);

    double latency = ( c2.tv_sec - c1.tv_sec ) + ( c2.tv_nsec - c1.tv_nsec ) / 1e9;

    if (!reg)
    {
    	sgx_destroy_enclave(global_eid);
    }
    
	printf("hashsize: %ld: reg: %d : request buffer size: %d : overall latency: %lf : untruested: %lf\n",
 HASHMAPSIZE * sizeof(item_t),
 reg + rpc*2,
 request_buf_size,
 latency,
acc_untrusted);

	char x[1];
	x[0] = 'e';
	int p = write(newsockfd,x,1);

    shutdown(newsockfd, SHUT_RDWR);
    shutdown(sockfd, SHUT_RDWR);
    close(newsockfd);
    close(sockfd);

    return 0;
}

int ocall_llc_test(int warmup)
{
	volatile int res =0;
	struct timespec c1,c2;
        clock_gettime(CLOCK_MONOTONIC, &c1);

		int counter = 0;
		while (counter < request_buf_size)
		{
			int n = read(newsockfd,request_buf + counter,request_buf_size - counter);
			if (n <= 0) {
			  perror("ERROR reading from socket");
			  exit(1);
		   }

			counter += n;
		}

	//   char x[1];
	//   x[0] = '\n';
	//   int p = write(newsockfd,x,1);

         clock_gettime(CLOCK_MONOTONIC, &c2);

	 if (!warmup)
	 {
                 counter_untrusted++;
		 double latency = ( c2.tv_sec - c1.tv_sec ) + ( c2.tv_nsec - c1.tv_nsec ) / 1e9;
		 acc_untrusted += latency;
	 }

	return res;
}

int fd1 =0;
void ocall_write_encrypted_data()
{
	if (fd1 == 0)
	{
		fd1 = open("/home/user/encrypted_requests", O_RDWR);
	}

	printf("%lld\n", (*(long long*)temp));

	write(fd1, temp, request_buf_size);
}
