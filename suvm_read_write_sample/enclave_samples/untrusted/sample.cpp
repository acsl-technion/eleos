#include <stdlib.h> 
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>
#undef NDEBUG
#include <assert.h>
#include <unistd.h>
#include <pwd.h>
#include <libgen.h>
#include <stdlib.h>

# define MAX_PATH FILENAME_MAX

#include <time.h>
#include <unistd.h>
#include <sgx_urts.h>
#include "sample.h"

#include "common/PerfCounters.h"
#include "samples_u.h"
#include "init_services.h"
#include <stdio.h>
#include <pthread.h>
#include <signal.h>

//#define MEASURE_PF

extern volatile unsigned long long *g_perf_counters;

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

volatile int thread_start = 0;
int num_of_threads;
int item_size;
int arr_size;
int test_read;
int test_write;
int test_type;
int avg_times;
int epcpp_size;


#include "memset.h"

volatile int g_test_stop;
volatile unsigned long g_timestamp;
#include "common/PerfCounters.h"
extern volatile unsigned long long *g_perf_counters;


# define HP_TIMING_NOW(Var) \
	({ unsigned int _hi, _lo; \
	 asm volatile ("rdtsc" : "=a" (_lo), "=d" (_hi)); \
	 (Var) = ((unsigned long long int) _hi << 32) | _lo; })

void* measure_single_pf(void* arg)  {
	unsigned long long read_begin, read_end;

	unsigned long long last_known_state = 1;
	unsigned long long counter = 1;

	while(1) {
		counter = READ_COUNTER_2(g_perf_counters);
		if(counter %2 == 0) {
			read_begin = 0;
		}
	}
}


void* measure_time(void* arg) {
#ifdef MEASURE_PF
	struct isgx_user_pages_param param;
	int sgx_fd = open("/dev/isgx", O_RDWR);

	param.set = 4;		// zeroing the pf counter
	int err = ioctl(sgx_fd, ISGX_IOCTL_USER_PAGES, &param);
	assert(!err);
	param.set = 3;		// for just reading the PF counter. Not changing it
#endif
	double avg=0;
	double min=(double)(1<<30);
	double max=-1;

	int prev_timestamp = g_timestamp;
	struct timespec t_prev;
	clock_gettime(CLOCK_REALTIME, &t_prev);
	struct timespec t;
	int num_iter=100;
	int nr_warmup_iter = 30;
	int nr_cooldown_iterations = 20;
	unsigned int nr_driver_pf1, nr_driver_pf2;
	unsigned long long nr_aprt_pf1, nr_aprt_pf2;

	for(int i=0;i<num_iter;i++) {
		while(1){
			int new_timestamp = g_timestamp;
			if(prev_timestamp != new_timestamp) {
				assert(new_timestamp - prev_timestamp == 1);
				clock_gettime(CLOCK_REALTIME, &t);
				//printf("%u\t%lf\n", g_timestamp, t.tv_sec + t.tv_nsec / 1e9);
				float tstpm=t.tv_sec - t_prev.tv_sec + (t.tv_nsec - t_prev.tv_nsec) / 1e9;

				unsigned long long rdtsc_read;
				HP_TIMING_NOW(rdtsc_read);
				printf("%u\t%lf pf:%llu enc_pf:%llu\trdtsc: %llu\n", g_timestamp, t.tv_sec - t_prev.tv_sec + (t.tv_nsec - t_prev.tv_nsec) / 1e9, PF_COUNT(g_perf_counters), ENCRYPT_COUNT(g_perf_counters), rdtsc_read);
				fflush(stdout);
				prev_timestamp = new_timestamp;
				t_prev = t;
				if (i==nr_warmup_iter) {
#ifdef MEASURE_PF
					int err = ioctl(sgx_fd, ISGX_IOCTL_USER_PAGES, &param);
					assert(!err);
					nr_driver_pf1 = param.quota;
					nr_aprt_pf1 = PF_COUNT(g_perf_counters);
					fprintf(stdout,"Starting averaging. #driver-PF = %d\n", nr_driver_pf1);
#endif
				}
				if (i==num_iter - nr_cooldown_iterations) {
#ifdef MEASURE_PF
					int err = ioctl(sgx_fd, ISGX_IOCTL_USER_PAGES, &param);
					assert(!err);
					nr_driver_pf2 = param.quota;
					nr_aprt_pf2 = PF_COUNT(g_perf_counters);
					fprintf(stdout,"Stopped averaging. #driver-PF = %d\n", nr_driver_pf2);
#endif
				}
				if (i>=nr_warmup_iter && i < num_iter - nr_cooldown_iterations){	// bad for 2 enclaves
					avg+=tstpm;
					if (tstpm<min) min=tstpm;
					if (tstpm>max) max=tstpm;
				}

				break;
			}
		}
	}
	fprintf(stdout,"RESULT: avg = %0.3f min = %0.3f max = %0.3f\t#driver-PF = %d\t#aptr-pf = %llu\n",avg/(num_iter-nr_warmup_iter-nr_cooldown_iterations),min,max, nr_driver_pf2 - nr_driver_pf1, nr_aprt_pf2 - nr_aprt_pf1);
	exit(0);
}


void* evaluate(void* arg)
{
	int ecall_return;
	thread_start++;

	while (thread_start < num_of_threads) {}

	switch (test_type)
	{
		case 1:
			process_native(arr_size, item_size, test_read, test_write, avg_times);
			break;
		case 2:
			ecall_process_reg(global_eid, &ecall_return, item_size, arr_size, test_read, test_write, avg_times, (long*)&g_timestamp, (int*)&g_test_stop);
			break;
		case 3:
			ecall_process_aptr(global_eid, &ecall_return, item_size, arr_size, test_read, test_write, avg_times, (long*)&g_timestamp, (int*)&g_test_stop);
			break;
	}

	__sync_fetch_and_add(&g_test_stop, 1);
}

/* Application entry */
int SGX_CDECL main(int argc, char *argv[])
{
	if (argc < 9)
	{
		printf("Usage: [THREAD_NUM] [ITEM_SIZE] [ARR_SIZE] [TEST_READ(0 false, 1 true)] [TEST_WRITE(0 false, 1 true)] [TEST_TYPE(1 native, 2 enclave_reg, 3 enclave_aptr)] [AVG_TIMES] [|EPC++| in power_of_two_pages]\n");
		return -1;
	}

	num_of_threads = atoi(argv[1]);
	item_size = atoi(argv[2]);
	arr_size = atoi(argv[3]);
	test_read = atoi(argv[4]);
	test_write = atoi(argv[5]);
	test_type = atoi(argv[6]);
	avg_times = atoi(argv[7]);

	g_test_stop = 0;
	printf("EPC size IS ALWAYS IGNORED : thread_num: %d : item_size : %d : arr_size: %d : read_test: %d : write_test: %d : test_type: %d \n",  num_of_threads, item_size, arr_size, test_read, test_write, test_type);

	if (test_type ==3 ){
		printf("EPC++ : %lu, buffer: %lu\n ",(unsigned long)CACHE_SIZE, arr_size*item_size*sizeof(int));

		if (CACHE_SIZE<arr_size*item_size*sizeof(int) ) printf("Should expect page faults\n");
		else printf("No page faults during the test!\n");
	}

	if (test_type != 1)
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

	int ecall_return = 0;

	switch (test_type)
	{
		case 1:
			preprocess_native(arr_size, item_size);
			break;
		case 2:
			ecall_preprocess_reg(global_eid, &ecall_return, item_size, arr_size);
			break;
		case 3:
			ecall_preprocess_aptr(global_eid, &ecall_return, item_size, arr_size);  	
			break;
	}

	printf("starting test\n");
	int test_time = 60; //10Sec
	unsigned int nr_driver_pf2 = 0;
	int err = 0;
#ifdef MEASURE_PF
	struct isgx_user_pages_param param;
	int sgx_fd = open("/dev/isgx", O_RDWR);

	param.set = 4;          // zeroing the pf counter
	err = ioctl(sgx_fd, ISGX_IOCTL_USER_PAGES, &param);
	assert(!err);
	param.set = 3;         
#endif
	pthread_t* work_threads = (pthread_t*)malloc(sizeof(pthread_t)*num_of_threads); 
	for (int i=0;i<num_of_threads;i++) {
		int res = pthread_create(&work_threads[i], NULL, evaluate, NULL);

		if(res) {
			printf("res = %d\n", res);
			exit(1);
		}

		// on the main thread: sleep and kill them, count how many buffers were used - measure throughput
	}

	// wait till all therads start to begin the test
	while (thread_start < num_of_threads) {}

	struct timespec c1,c2;
	clock_gettime(CLOCK_REALTIME, &c1);

	while (g_test_stop < num_of_threads) {}

	clock_gettime(CLOCK_REALTIME, &c2);
	double latency = ( c2.tv_sec - c1.tv_sec ) + ( c2.tv_nsec - c1.tv_nsec ) / 1e9;

	// now can measure number of operations, from that the avg throughput and latency..
#ifdef MEASURE_PF
	err = ioctl(sgx_fd, ISGX_IOCTL_USER_PAGES, &param);
	assert(!err);
	nr_driver_pf2 = param.quota;
#endif
	printf("Did : %ld ops, time passed: %d sec. Avg Throughput: %lf, Avg Latency: %lf, #driver-PF %d, SUVM-PF %lld\n", g_timestamp, test_time, g_timestamp/latency,latency/g_timestamp, nr_driver_pf2, *g_perf_counters);

	if (test_type !=1)
	{
		sgx_destroy_enclave(global_eid);
	}

	if (cleanup_lib(global_eid) != 0)
	{
		printf("cleanup failed for enclave: %lu\n", global_eid);
		abort();
	}

	exit(ecall_return);
}
