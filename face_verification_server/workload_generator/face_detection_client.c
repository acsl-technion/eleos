#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <sgx_tcrypto.h>
//#include <sgx_trts.h>

#include <errno.h>

static void * (* const volatile __memset_vp)(void *, int, size_t)
    = (memset);

#ifdef memset_s
#undef memset_s /* in case it was defined as a macro */
#endif

#ifdef __cplusplus
extern "C"
#endif
int memset_s(void *s, size_t smax, int c, size_t n)
{
    int err = 0;

    if (s == NULL) {
        err = EINVAL;
        goto out;
    }

    if (n > smax) {
        err = EOVERFLOW;
        n = smax;
    }

    /* Calling through a volatile pointer should never be optimised away. */
    (*__memset_vp)(s, c, n);

    out:
    if (err == 0)
        return 0;
    else {
        errno = err;
        /* XXX call runtime-constraint handler */
        return err;
    }
}

#ifdef __cplusplus
extern "C"
#endif
int
consttime_memequal(const void *b1, const void *b2, size_t len)
{
	const unsigned char *c1 = (const unsigned char*)b1, *c2 = (const unsigned char*)b2;
	unsigned int res = 0;

	while (len--)
		res |= *c1++ ^ *c2++;

	/*
	 * Map 0 to 1 and [1, 256) to 0 using only constant-time
	 * arithmetic.
	 *
	 * This is not simply `!res' because although many CPUs support
	 * branchless conditional moves and many compilers will take
	 * advantage of them, certain compilers generate branches on
	 * certain CPUs for `!res'.
	 */
	return (1 & ((res - 1) >> 8));
}

extern "C" sgx_status_t sgx_init_crypto_lib(uint64_t cpu_feature_indicator);

#define DUP_NUM 2
#define NUM_OF_REQUESTS 10000
#define NUM_OF_CLIENTS 100

#define HISTOGRAM_LEN 58
#define IMG_WIDTH 512
#define BLOCK_WIDTH 16
#define NBLOCKS_DIM (IMG_WIDTH / BLOCK_WIDTH)
#define NBLOCKS (NBLOCKS_DIM * NBLOCKS_DIM)

#define ROT_INVARIANT

unsigned char get_pattern(unsigned char img[IMG_WIDTH][IMG_WIDTH], int r, int c) {
	unsigned char v = img[r][c];
	unsigned char pattern = ((img[r - 1][c - 1] > v) << 7) |
		((img[r - 1][c    ] > v) << 6) |
		((img[r - 1][c + 1] > v) << 5) |
		((img[r    ][c + 1] > v) << 4) |
		((img[r + 1][c + 1] > v) << 3) |
		((img[r + 1][c    ] > v) << 2) |
		((img[r + 1][c - 1] > v) << 1) |
		((img[r    ][c - 1] > v) << 0);
	return pattern;
}

int is_pattern_uniform(unsigned char pattern) {
	unsigned char last = pattern & 1;
	unsigned char cur = pattern;
	int transitions = 0;
	int i;
	for (i = 0; i < 8; i++) {
		if ((cur & 1) != last) {
			transitions ++;
		}
		last = cur & 1;
		cur >>= 1;
	}
	if ((pattern & 1) != ((pattern >> 7) & 1)) /* circular transition */
		transitions++;
	return transitions < 3;
}

unsigned char rotate(unsigned char pattern, int rotation) {
	unsigned char cur = pattern;
	int i;
	unsigned char lsb;
	for (i = 0; i < rotation; i++) {
		lsb = cur & 1;
		cur >>= 1;
		cur |= (lsb << 7);
	}
	return cur;
}

int create_pattern2bin(int *pattern2bin) {
	int i;
	int next;
	next = 0;
	for (i = 0; i < 256; i++)
		pattern2bin[i] = 0xff;

	for (i = 0; i < 256; i++) {
		if (pattern2bin[i] != 0xff)
			continue;

		if (is_pattern_uniform(i)) {
			pattern2bin[i] = next;
#ifdef ROT_INVARIANT
			int j;
			for (j = 0; j < 8; j++) {
				pattern2bin[rotate(i, j)] = next;
			}
#endif
			next++;
		} else {
			pattern2bin[i] = HISTOGRAM_LEN - 1;
		}
	}
	/*if (next != HISTOGRAM_LEN)
	  printf("expected %d bins. got %d\n", HISTOGRAM_LEN, next);*/
	return 0;
}

struct req_t {
	unsigned char name[40];
#ifdef READ_REQ_FROM_FILES
	char fname[40];
#else
	int hist[HISTOGRAM_LEN * NBLOCKS];	
	//unsigned char image[IMG_WIDTH][IMG_WIDTH];
#endif
};

volatile int false_positives = 0;
volatile int false_negatives = 0;
volatile int true_positives = 0;
volatile int true_negatives = 0;
struct sockaddr_in serv_addr;
struct req_t *req_array;
volatile int thread_start_count = 0;
volatile int g_num_of_requests_answered = 0;
volatile int thread_index_id=0;

sgx_aes_ctr_128bit_key_t aes_key = {0};
uint8_t ctr[16*100] = {0};

static void* enc_thread_start(void *arg)
{
	int sockfd = 0;
	size_t hist_size = sizeof(int) * HISTOGRAM_LEN * NBLOCKS;

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Error : Could not create socket \n");
		return NULL;
	}
	if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("\n Error : Connect Failed \n");
		return NULL;
	}

	unsigned char* data = (unsigned char*)arg;
	__sync_fetch_and_add(&thread_start_count,1);

	while (thread_start_count < NUM_OF_CLIENTS) {}

	for (int i=0; i<NUM_OF_REQUESTS/NUM_OF_CLIENTS; i++) {

		int n;
		for (int offset=0; offset<hist_size+40+sizeof(int); offset+=16384)
		{
			int size_to_write = 16384;
			if (hist_size - offset < 16384) size_to_write = (hist_size+40+sizeof(int)) - offset;
			n = write(sockfd, &data[offset], size_to_write);
		}

		int answer = 999;
		n = read(sockfd, &answer, sizeof(int));
		int decrypted_answer;
		int temp_id = *(int*)data;
		sgx_status_t y = sgx_aes_ctr_decrypt(&aes_key,
				(unsigned char*)&answer,
				sizeof(int),
				&ctr[temp_id*16],
				8,
				(unsigned char*)&decrypted_answer);
		if (y != 0) {
			printf("failed to decrypt answer\n"); exit(-1);
		}

		if (decrypted_answer != 0) __sync_fetch_and_add(&true_positives,1);
		else __sync_fetch_and_add(&false_positives,1);
		/*
		   if (correct_name && answer) __sync_fetch_and_add(&true_positives,1);
		   if (!correct_name && answer) __sync_fetch_and_add(&false_positives,1);
		   if (!correct_name && !answer) __sync_fetch_and_add(&true_negatives,1);
		   if (correct_name && !answer) __sync_fetch_and_add(&false_negatives,1);
		 */
		__sync_fetch_and_add(&g_num_of_requests_answered,1);

		data += hist_size+40+sizeof(int);
	}

	close(sockfd);
}

int pattern2bin[256];

int main(int argc, char *argv[])
{
	srand(time(0));	
	create_pattern2bin(pattern2bin);
	
	if (argc != 2) {
		printf("Usage: ./client [PATH_TO_ENCRYPTED_DATASET_FILE]\n");
		exit(-1);
	}

	double total_time = 0;
	int n = 0; int i;
	struct timespec c1, c2;

	size_t buff_size = sizeof(int) * HISTOGRAM_LEN * NBLOCKS+40+sizeof(int);
	unsigned char* buff = (unsigned char*)malloc(buff_size);
	unsigned char* requests_arr[NUM_OF_CLIENTS];
	int client_requests_loaded[NUM_OF_CLIENTS];
	for (int i=0;i<NUM_OF_CLIENTS;i++)
	{
		requests_arr[i] = (unsigned char*)malloc(buff_size*NUM_OF_REQUESTS/NUM_OF_CLIENTS);
		client_requests_loaded[i] = 0;
	}

	int fd = open(argv[1], O_RDONLY);

	printf("Opened file\n");

	int buf_idx = 0;
	int items_read = 0;
	do
	{
		n = read(fd, buff, buff_size);
		if (n == buff_size)
		{
			n = 0;
			items_read++;
			int client_id = *(int*)buff;
			memcpy(requests_arr[client_id]+client_requests_loaded[client_id]*buff_size, buff, buff_size);
			client_requests_loaded[client_id]++;
		}
	} while (items_read < NUM_OF_REQUESTS);

	printf("loaded requesets\n");

	memset(&serv_addr, '0', sizeof(serv_addr)); 

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(5006); 

	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
	{
		printf("\n inet_pton error occured\n");
		return 1;
	}

	printf("starting therads...\n");

	pthread_t tds[NUM_OF_CLIENTS];
	for(i=0;i<NUM_OF_CLIENTS;i++) {
		if(pthread_create(&tds[i], NULL, enc_thread_start, requests_arr[i])) {
			fprintf(stderr, "Error creating thread\n");
			return 1;
		}
	}

	while (thread_start_count != NUM_OF_CLIENTS) {}

	printf("configured..starting test\n");
	clock_gettime(CLOCK_MONOTONIC, &c1);

	for(i=0;i<NUM_OF_CLIENTS;i++) {
		if(pthread_join(tds[i], NULL)) {
			fprintf(stderr, "Error joining thread\n");
			return 1;
		}
	}

	clock_gettime(CLOCK_MONOTONIC, &c2);
	double latency = ( c2.tv_sec - c1.tv_sec ) + ( c2.tv_nsec - c1.tv_nsec ) / 1e9;
	total_time = latency;

	printf("true positives %.1f %% (%d / %d)\n", (float)true_positives / NUM_OF_REQUESTS * 100, true_positives, NUM_OF_REQUESTS);
	printf("true negatives %.1f %% (%d / %d)\n", (float)true_negatives / NUM_OF_REQUESTS * 100, true_negatives, NUM_OF_REQUESTS);
	printf("false positives %.1f %% (%d / %d)\n", (float)false_positives / NUM_OF_REQUESTS * 100, false_positives, NUM_OF_REQUESTS);
	printf("false negatives %.1f %% (%d / %d)\n", (float)false_negatives / NUM_OF_REQUESTS * 100, false_negatives, NUM_OF_REQUESTS);
	printf("\n\nTroughput: %.1f [Reqs/sec]. Test Time: %lf\n", (float)NUM_OF_REQUESTS/total_time, total_time);

	return 0;
}
