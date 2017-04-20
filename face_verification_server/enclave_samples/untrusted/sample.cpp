#include <string.h>
#include "server.h"
#include <assert.h>

#include <unistd.h>
#include <pwd.h>
#include <libgen.h>
#include <stdlib.h>
#include "face_verification/face_verification.h"

# define MAX_PATH FILENAME_MAX

#include <time.h>
#include <unistd.h>
#include <sgx_urts.h>
#include "sample.h"
#include "../common/Queue.h"

#include "samples_u.h"
#include "init_services.h"
#include "../TestsEnum.h"
#include <argp.h>
#include <stdio.h>
#include <pthread.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../common_structs.h"

#define NUM_OF_THREADS 4
#define IMG_WIDTH 512

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
		printf("Error: Unexpected error occurred 0x%x.\n", ret);
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
	//printf("token_path: %s\n", token_path);
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

	ret = sgx_create_enclave(SAMPLES_FILENAME, SGX_DEBUG_FLAG, &token, &updated, &global_eid, NULL);

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
void ocall_samples_sample(const char *str)
{
	/* Proxy/Bridge will check the length and null-terminate 
	 * the input string to prevent buffer overflow. 
	 */
	printf("%s", str);
}

const char *argp_program_version =
"Exit-less services for SGX version 0.1";
const char *argp_program_bug_address =
"<shmenio@gmail.com>";

static char doc[] = "Testing Framework for measuring performance of sgx exit-less service lib";

/* A description of the arguments we accept. */
static char args_doc[] = "";

/* The options we understand. */
static struct argp_option options[] = {
	{"test_lib",  'l',  0,      OPTION_ARG_OPTIONAL,  "Should test using the lib?" },
	{"test_name",   't', "NUM", OPTION_ARG_OPTIONAL, "Name of the test (look at TestsEnum.h)" },
	{"item_size",    'a', "NUM",      OPTION_ARG_OPTIONAL,  "The number of insertions for hashmap test" },
	{ 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments
{
	char *args[3];                /* arg1 & arg2 */
	char* item_size;
	int is_testing_lib;
	char *test_name;
};

/* Parse a single option. */
	static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
	/* Get the input argument from argp_parse, which we
	   know is a pointer to our arguments structure. */
	struct arguments *arguments = (struct arguments *)state->input;
	switch (key)
	{
		case 'l':
			arguments->is_testing_lib = 1;
			break;
		case 'a':
			arguments->item_size = arg;
			break;
		case 't':
			arguments->test_name = arg;
			break;

		case ARGP_KEY_ARG:
			if (state->arg_num >= 3)
			{
				/* Too many arguments. */
				printf("to many\n");
				argp_usage (state);

			}

			arguments->args[state->arg_num] = arg;

			break;

		case ARGP_KEY_END:
			if (state->arg_num < 0)
			{
				/* Not enough arguments. */

				printf("not enough\n");
				argp_usage (state);
			}
			break;

		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc };

TestsIds testId;
bool testWithLib;

void* evaluate_thread(void* arg)
{
	int ecall_return;
	int testId = 0; // face verification
	// Blocking call, endless call
	if (!testWithLib)
		ecall_evaluate_regular(global_eid, &ecall_return, testId, arg);
	else
		workload_face_verification(arg);	
}

void my_server()
{
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr;

	time_t ticks;

	char key[1024];	
	char img[IMG_WIDTH * IMG_WIDTH];

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(5005);

	bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

	listen(listenfd, 128);
	int i=0;
	while (1)
	{
		connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

		int n = read(connfd, key, 40);
		int total_read = 0, n2 = 0;
		while ( total_read < IMG_WIDTH*IMG_WIDTH)
		{
			n2 = read(connfd, &img[total_read], IMG_WIDTH*IMG_WIDTH);
			total_read += n2;
		}
		printf("req: %d read: %d recv: %s read2:%d\n",i++, n, key, total_read);

		write(connfd, key, n);

		close(connfd);
	}	
}

Queue* g_request_queue;
Queue* g_response_queue;

int do_trusted_logic(unsigned char* data, void* client_data)
{
	// Put in request queue
	request* req = new request(); // will get freed in response queue
	request* response = new request();
	s_server_data* send_data = new s_server_data();
	send_data->_buffer = data;
	send_data->_client_data = client_data;
	send_data->_response = response;
	response->buffer = req;
	req->buffer = send_data;
	g_request_queue->enqueue(req);
	return 0;
	/*
	   int ecall_return;
	   int testId = 6; // face verification
	   ecall_evaluate_regular(global_eid, &ecall_return, testId, data);
	   return ecall_return;
	 */
}

void* send_answer(bool* is_empty, int* answer)
{
	// Get answer from queue, then return it so the server will send it.
	request* resp = g_response_queue->dequeue();
	if (resp == NULL)
	{
		*is_empty = true;
		return NULL;
	}

	*is_empty = false;
	*answer = resp->is_done;
	request* orig_request = (request*)resp->buffer;
	s_server_data* resp_data = (s_server_data*)orig_request->buffer;
	void* client_data = resp_data->_client_data;
	delete (resp_data);
	delete (orig_request);
	delete (resp);
	return client_data;
}

/* Application entry */
int SGX_CDECL main(int argc, char *argv[])
{
	struct arguments arguments;

	/* Default values. */
	arguments.is_testing_lib = 0;
	arguments.item_size = "-";
	arguments.test_name = "-";

	/* Parse our arguments; every option seen by parse_opt will
	   be reflected in arguments. */
	argp_parse (&argp, argc, argv, 0, 0, &arguments);

	g_request_queue = new Queue();
	g_response_queue = new Queue();

	testWithLib = arguments.is_testing_lib;
	testId = (TestsIds)atoi(arguments.test_name);
	long long numOfItems = atoll(arguments.item_size);
	void* arg = &numOfItems;
	int test_read = 1;
	int test_write = 1;

	/* Changing dir to where the executable is.*/
	char absolutePath [MAX_PATH];
	char *ptr = NULL;

	ptr = realpath(dirname(argv[0]),absolutePath);

	if( chdir(absolutePath) != 0)
		abort();

	sgx_status_t ret = SGX_ERROR_UNEXPECTED;
	int ecall_return = 0;

	if (!testWithLib) 
	{

		/* Initialize the enclave */
		if(initialize_enclave() < 0){
			printf("failed to init enclave\n");
			return -1; 
		}

		if (initialize_lib(global_eid,/*rpc=*/false) != 0)
		{
			printf("initialization failed for enclave: %lu\n", global_eid);
			abort();
		}
	}

	printf("Starting to preprocess\n");

	if (testWithLib)
	{
		face_verification_prepare_server();
	}
	else
	{
		ret = ecall_evaluate_regular_preprocess(global_eid, &ecall_return, testId, g_request_queue, g_response_queue);
		if (ret != SGX_SUCCESS)
		{
			abort();
		}
	}


	pthread_t* work_threads = (pthread_t*)malloc(sizeof(pthread_t)*NUM_OF_THREADS);
	for (int i=0;i<NUM_OF_THREADS;i++) {
		int res = pthread_create(&work_threads[i], NULL, evaluate_thread, NULL);

		if(res) {
			printf("res = %d\n", res);
			exit(1);
		}
		// on the main thread: sleep and kill them, count how many buffers were used - measure throughput
	}

	printf("Server running, end with Ctrl+C\n");
	runServer(do_trusted_logic, send_answer); // this will start the server...
	// Blocking call. Server will be killed with Ctrl+C
}
