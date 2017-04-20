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

#define ACCEPT_THRESHOLD 120000.0

#define IMAGE_FILES_PATH "./images_raw/"

#define to_mat(x) (unsigned char (*)[IMG_WIDTH])(x)

void * my_malloc(size_t size) {
	void *ptr = malloc(size);
	if (!ptr) {
		perror("malloc");
		exit(1);
	}
	return ptr;
}

void * my_realloc(void *ptr, size_t old_size, size_t new_size) {
	void *new_ptr = my_malloc(new_size);
	memset(new_ptr, 0, new_size);
	memcpy(new_ptr, ptr, old_size);
	free(ptr);
	return new_ptr;
}

struct hashmap_entry_t {
	unsigned char key[40];
	long value_offset;
	int size;
};

struct hashmap_t {
	struct hashmap_entry_t *index;
	unsigned char *buffer;
	long current_offset;
	size_t total_size;
	size_t nentries;
	size_t nbuckets;
};

void hashmap_init(struct hashmap_t *m);
void hashmap_destroy(struct hashmap_t *m);
void hashmap_get(struct hashmap_t *m, unsigned char *key, unsigned char **val, int *size);
void hashmap_put(struct hashmap_t *m, unsigned char *key, unsigned char *val, int size, int override);
void hashmap_rehash(struct hashmap_t *m);

long hash_function(unsigned char *str, int nbuckets) {
	unsigned long hash = 5381;
	int c;

	while ((c = *str++)) {
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}

	return hash % nbuckets;
}

void hashmap_init(struct hashmap_t *m) {
	size_t nbuckets_initial = 128;
	size_t buffer_size_initial = 1024;
	m->index = (struct hashmap_entry_t*)my_malloc(sizeof(struct hashmap_entry_t) * nbuckets_initial);
	m->buffer = (unsigned char*)my_malloc(sizeof(unsigned char) * buffer_size_initial);

	int i;
	for (i = 0; i < nbuckets_initial; i++) {
		m->index[i].value_offset = -1;
	}

	m->current_offset = 0;
	m->total_size = buffer_size_initial;
	m->nentries = 0;
	m->nbuckets = nbuckets_initial;
}

void hashmap_destroy(struct hashmap_t *m) {
	free(m->index);
	free(m->buffer);
}

void hashmap_rehash(struct hashmap_t *m) {
	if (m->nentries * 2 < m->nbuckets) return;
	size_t nbuckets_new = 2 * m->nbuckets;
	struct hashmap_entry_t *new_index = (struct hashmap_entry_t*)my_malloc(sizeof(struct hashmap_entry_t) * nbuckets_new);
	int i;
	for (i = 0; i < nbuckets_new; i++) {
		new_index[i].value_offset = -1;
	}

	for (i = 0; i < m->nbuckets; i++) {
		if (m->index[i].value_offset >= 0) {
			int hash = hash_function(m->index[i].key, nbuckets_new);
			while (new_index[hash].value_offset >= 0) {
				hash = (hash + 1) % nbuckets_new;
			}
			strcpy((char *)new_index[hash].key, (char *)m->index[i].key);
			new_index[hash].value_offset = m->index[i].value_offset;
			new_index[hash].size = m->index[i].size;
		}
	}
	m->nbuckets = nbuckets_new;
	free(m->index);
	m->index = new_index;
}

void hashmap_get(struct hashmap_t *m, unsigned char *key, unsigned char **val, int *size) {
	int hash = hash_function(key, m->nbuckets);
	int idx = hash;
	struct hashmap_entry_t *entry;
	do {
		entry = &m->index[idx];
		if ((entry->value_offset >= 0) && (!strcmp((char *)key, (char *)m->index[idx].key))) {
			*val = m->buffer + m->index[idx].value_offset;
			if (size) *size = m->index[idx].size;
			return;
		}
		idx = (idx + 1) % m->nbuckets;
	} while ((idx != hash) && (entry->value_offset >= 0));
	*val = NULL;
	if (size) *size = 0;
}

void hashmap_put(struct hashmap_t *m, unsigned char *key, unsigned char *val, int size, int override) {
	unsigned char *old_val;
	hashmap_get(m, key, &old_val, NULL);
	if (old_val && !override) {
		return;
	}

	if ((m->nentries + 1) * 2 > m->nbuckets) hashmap_rehash(m);

	while (m->current_offset + size > m->total_size) {
		m->buffer = (unsigned char*)my_realloc(m->buffer, m->total_size, m->total_size * 2);
		m->total_size *= 2;
	}

	memcpy((void *)((intptr_t)m->buffer + m->current_offset), val, size);
	int hash = hash_function(key, m->nbuckets);

	int idx = hash;
	while ((m->index[idx].value_offset >= 0) && strcmp((char *)m->index[idx].key, (char *)key)) {
		idx = (idx + 1) % m->nbuckets;
	}

	struct hashmap_entry_t *entry = &m->index[idx];
	strcpy((char *)entry->key, (char *)key);
	entry->value_offset = m->current_offset;
	entry->size = size;

	m->current_offset += size;
	m->nentries++;
}


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

float histogram_distance(int *h1, int *h2) {
	float chisqr = 0;
	int diff;
	int sum;
	int i;
	#pragma omp simd
	for (i = 0; i < HISTOGRAM_LEN * NBLOCKS; i++) {
		printf("%d <=> %d\n", h1[i], h2[i]);
		diff = h1[i] - h2[i];
		sum = h1[i] + h2[i];
		if (sum != 0) chisqr += (float)(diff * diff) / sum;
	}
	abort();
	return chisqr;
}

void calc_histogram(int *histogram, unsigned char img[IMG_WIDTH][IMG_WIDTH], int *pattern2bin) {
	memset(histogram, 0, sizeof(int) * HISTOGRAM_LEN * NBLOCKS);

	int i, j;
	for (i = 1; i < IMG_WIDTH - 1; i++) {
		for (j = 1; j < IMG_WIDTH - 1; j++) {
			int pattern = get_pattern(img, i, j);
			int histogram_i =  i / BLOCK_WIDTH;
			int histogram_j = j / BLOCK_WIDTH;
			int start_offset = histogram_i * NBLOCKS_DIM * HISTOGRAM_LEN + histogram_j * HISTOGRAM_LEN;
			histogram[start_offset + pattern2bin[pattern]]++;
		}
	}

}

float do_one(struct hashmap_t *hashmap, int *histogram, int *pattern2bin, unsigned char img[IMG_WIDTH][IMG_WIDTH], unsigned char *key) {
	int *saved_histogram;

	calc_histogram(histogram, img, pattern2bin);

	hashmap_get(hashmap, key, (unsigned char **)&saved_histogram, NULL);

	float distance = histogram_distance(histogram, saved_histogram);
	return distance;
}

int populate_database(struct hashmap_t *hashmap, char *path, int *pattern2bin) {
	int n;
	int ret;
	char fname[40];
	int cnt = 0;
	int histogram[HISTOGRAM_LEN * NBLOCKS];

	uint64_t mask = 0;
        mask |= 0x00000001;
        mask |= 0x00000002;
        mask |= 0x00000004;
        mask |= 0x00000008;
        mask |= 0x00000010;
        mask |= 0x00000020;
        mask |= 0x00000040;
        mask |= 0x00000200;
        mask |= 0x00000800;
        mask |= 0x00002000;
        mask |= 0x00004000;
        mask |= 0x00008000;
        mask |= 0x00010000;
        mask |= 0x00020000;
        mask |= 0x00040000;

        mask |= 0x00000400; //aes;
        mask |= 0x00000080; //sse4.2
        mask |= 0x00000100; //avx;
        fprintf(stderr,"%ld\n", mask);
        sgx_init_crypto_lib(mask);

	unsigned char *img = (unsigned char*)my_malloc(IMG_WIDTH * IMG_WIDTH);

	DIR *d = opendir(path);
	if (!d) {
		perror("opendir");
		exit(1);
	}
	struct dirent *dent;

	size_t already_added_size = 1024;
	char *already_added = (char*)my_malloc(already_added_size);

	while ((dent = readdir(d))) {
		struct stat st;
		unsigned char *key;
		sprintf(fname, "%s/%s", path, dent->d_name);
		stat(fname, &st);
		if ((st.st_mode & S_IFMT) == S_IFDIR)
			continue;
		key = (unsigned char *)strtok(dent->d_name, "_");
		int key_int = atoi((char *)key);
		if (key_int >= already_added_size) {
			already_added = (char*)my_realloc(already_added, already_added_size, already_added_size * 2);
			already_added_size *= 2;
		}
		if (already_added[key_int]) {
			continue;
		}
		already_added[key_int] = 1;
		int fd = open(fname, O_RDONLY);
		if (fd < 0) {
			perror("open");
			printf("%d::%s\n", __LINE__, fname);
			exit(1);
		}
		n = 0;
		while (n < IMG_WIDTH * IMG_WIDTH) {
			ret = read(fd, img + n, IMG_WIDTH * IMG_WIDTH - n);
			if (ret > 0) {
				n += ret;
			} else {
				perror("image read");
				printf("%d::%s\n", __LINE__, fname);
				exit(1);
			}
		}
		close(fd);
		calc_histogram(histogram, to_mat(img), pattern2bin);
		hashmap_put(hashmap, key, (unsigned char *)histogram, sizeof(int) * HISTOGRAM_LEN * NBLOCKS, 0);
		cnt++;
#ifdef MAX_DB_IMAGES
		if (cnt == MAX_DB_IMAGES)
			break;
#endif
	}
	int total_db_size_bytes = cnt * sizeof(int) * HISTOGRAM_LEN * NBLOCKS;
	printf("total DB size %d [B]  %f.1 [MB]\n", total_db_size_bytes, (float)total_db_size_bytes / (1024 * 1024));
	free(img);
	free(already_added);
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

int load_requests(struct req_t **preq_array, char *path, int *pattern2bin) {
	printf("Loading requests");
	
	int i=0;
	size_t req_array_size = 1024;
	struct req_t *req_array = (struct req_t*)my_malloc(sizeof(struct req_t) * req_array_size);

	DIR *d = opendir(path);
	if (!d) {
		perror("opendir");
		exit(1);
	}
	struct dirent *dent;

	char fname[200];
	unsigned char *img = (unsigned char*)my_malloc(IMG_WIDTH * IMG_WIDTH);
	size_t cnt = 0;
	while ((dent = readdir(d))) {
		i++; if (i%1000==0) printf(".");
		struct stat st;
		unsigned char *key;
		sprintf(fname, "%s/%s", path, dent->d_name);
		stat(fname, &st);
		if ((st.st_mode & S_IFMT) == S_IFDIR)
			continue;

		if (cnt == req_array_size) {
			req_array = (struct req_t*)my_realloc(req_array, sizeof(struct req_t) * req_array_size, sizeof(struct req_t) * req_array_size * 2);
			req_array_size *= 2;
		}

#ifdef READ_REQ_FROM_FILES
		strcpy((char *)req_array[cnt].fname, fname);
#else
		int fd = open(fname, O_RDONLY);
		if (fd < 0) {
			perror("open");
			printf("%d::%s\n", __LINE__, fname);
			exit(1);
		}
		int n = 0;
		while (n < IMG_WIDTH * IMG_WIDTH) {
			int ret = read(fd, img + n, IMG_WIDTH * IMG_WIDTH - n);
			if (ret > 0) {
				n += ret;
			} else {
				perror("image read");
				printf("%d::%s\n", __LINE__, fname);
				exit(1);
			}
		}
		close(fd);
		key = (unsigned char *)strtok(dent->d_name, "_");
		calc_histogram(req_array[cnt].hist, to_mat(img), pattern2bin);
		strcpy((char *)req_array[cnt].name, (char*)key);
#endif
		cnt++;
	}
	free(img);

	printf("done with cnt: %ld\n", cnt);

	*preq_array = req_array;
	return cnt;
}
/*
   int get_request(struct req_t *req_array, int req_array_len, unsigned char **img, unsigned char **key) {
   int req_idx = rand() % req_array_len;
   int name_idx;
   int correct_name = rand() % 2;
   if (correct_name) {
   name_idx = req_idx;
   } else {
   name_idx = rand() % req_array_len;
   }

 *key = req_array[name_idx].name;
#ifdef READ_REQ_FROM_FILES
int fd = open(req_array[req_idx].fname, O_RDONLY);
if (fd < 0) {
perror("open");
printf("%d::%s\n", __LINE__, req_array[req_idx].fname);
exit(1);
}
int n = 0;
int ret;
while (n < IMG_WIDTH * IMG_WIDTH) {
ret = read(fd, *img + n, IMG_WIDTH * IMG_WIDTH - n);
if (ret > 0) {
n += ret;
} else {
perror("image read");
printf("%d::%s\n", __LINE__, req_array[req_idx].fname);
exit(1);
}
}
close(fd);
#else
 *img = (unsigned char *)req_array[req_idx].image;
#endif
return correct_name;
}*/


int get_request(struct req_t *req_array, int req_array_len, unsigned char **histogram, unsigned char **key) {
	int req_idx = rand() % req_array_len;

	int name_idx;
	int correct_name = rand() % 2;
	//int correct_name = 1;
	if (correct_name) {
		name_idx = req_idx;
	} else {
		name_idx = rand() % req_array_len;
	}
	//printf("req_array_len: %d req_idx: %d correct_name: %d name_idx: %d\n", req_array_len, req_idx, correct_name, name_idx);
	unsigned char* tmp = req_array[name_idx].name;
	int dup_id=rand()%DUP_NUM;
	sprintf((char*)*key, "%s%d", tmp, dup_id);

#ifdef READ_REQ_FROM_FILES
	int fd = open(req_array[req_idx].fname, O_RDONLY);
	if (fd < 0) {
		perror("open");
		printf("%d::%s\n", __LINE__, req_array[req_idx].fname);
		exit(1);
	}
	int n = 0;
	int ret;
	while (n < IMG_WIDTH * IMG_WIDTH) {
		ret = read(fd, *img + n, IMG_WIDTH * IMG_WIDTH - n);
		if (ret > 0) {
			n += ret;
		} else {
			perror("image read");
			printf("%d::%s\n", __LINE__, req_array[req_idx].fname);
			exit(1);
		}
	}
	close(fd);
#else
	*histogram = (unsigned char*)req_array[req_idx].hist;
#endif
	return correct_name;
}
/*
void do_loop(struct hashmap_t *hashmap, char *path, int *pattern2bin, int nrequests) {

	struct req_t *req_array;
	//int cnt = load_requests(&req_array, path);
	load_requests(&req_array, path, pattern2bin);

	unsigned char *img;
	unsigned char *key;
	int *histogram = (int*)my_malloc(sizeof(int) * HISTOGRAM_LEN * NBLOCKS);

#ifdef READ_REQ_FROM_FILES
	img = malloc(IMG_WIDTH * IMG_WIDTH);
#endif

	int i;
	float distance;
	int false_positives = 0;
	int false_negatives = 0;
	int true_positives = 0;
	int true_negatives = 0;
	double distance_match = 0;
	double distance_mismatch = 0;
	for (i = 0; i < nrequests; i++) {
		//int correct_name = get_request(req_array, cnt, &img, &key);
		int correct_name = get_request(req_array, i, &img, &key);
		distance = do_one(hashmap, histogram, pattern2bin, to_mat(img), key);
		printf("should match? %s   distance %f\n", correct_name ? "YES" : "NO", distance);
		if (distance < ACCEPT_THRESHOLD) {
			if (correct_name) true_positives++;
			else              false_positives++;
		} else {
			if (correct_name) false_negatives++;
			else              true_negatives++;
		}
		if (correct_name) distance_match    += distance;
		else              distance_mismatch += distance;
	}

	distance_match    /= (true_positives + false_negatives);
	distance_mismatch /= (true_negatives + false_positives);

	printf("true positives %.1f %% (%d / %d)\n", (float)true_positives / nrequests * 100, true_positives, nrequests);
	printf("true negatives %.1f %% (%d / %d)\n", (float)true_negatives / nrequests * 100, true_negatives, nrequests);
	printf("false positives %.1f %% (%d / %d)\n", (float)false_positives / nrequests * 100, false_positives, nrequests);
	printf("false negatives %.1f %% (%d / %d)\n", (float)false_negatives / nrequests * 100, false_negatives, nrequests);
	printf("average distance match %lf   mismatch %lf\n", distance_match, distance_mismatch);
	free(histogram);
	free(req_array);
}

   int main(int argc, char *argv[]) {

   int nrequests = 11338;

//srand(time(0));
srand(0);
int pattern2bin[256];
create_pattern2bin(pattern2bin);

struct hashmap_t hashmap;
hashmap_init(&hashmap);

printf("populating database\n");
populate_database(&hashmap, IMAGE_FILES_PATH, pattern2bin);
printf("done populating database\n");

printf("starting requests\n");
do_loop(&hashmap, IMAGE_FILES_PATH, pattern2bin, nrequests);
printf("done\n");

hashmap_destroy(&hashmap);

return 0;
}*/


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

static void* thread_start(void *arg)
{
	int cnt = *(int*)arg;
	int sockfd = 0;
	unsigned char *histogram;
	unsigned char *key = (unsigned char*)malloc(40);
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
	
	int temp_id = thread_index_id++;
	printf("temp_id=%d\n",temp_id);
	__sync_fetch_and_add(&thread_start_count,1);

	unsigned char* encrypted_buffer = (unsigned char*)malloc(40+hist_size+sizeof(int));
	unsigned char* plain_buffer = (unsigned char*)malloc(40+hist_size);

	while (thread_start_count < NUM_OF_CLIENTS) {}

	int fd1 = open("encrypted_dataset.txt", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);

	for (int i=0; i<NUM_OF_REQUESTS/NUM_OF_CLIENTS; i++) {
		temp_id = (temp_id + 1) % 100;
		int correct_name = get_request(req_array, cnt, &histogram, &key);
		memcpy(plain_buffer, key, 40);
		memcpy(plain_buffer+40, histogram, hist_size);

		memcpy(encrypted_buffer,&temp_id,sizeof(int));

                sgx_status_t y = sgx_aes_ctr_encrypt(&aes_key,
                                                     plain_buffer,
                                                     (unsigned int)40+hist_size,
                                                     &ctr[temp_id*16],
                                                     8,
                                                     encrypted_buffer+sizeof(int));

		//memcpy(encrypted_buffer+sizeof(int), plain_buffer, 40+hist_size);
		//int n = write(sockfd, key, 40*sizeof(char));
		int n;	
		for (int offset=0; offset<hist_size; offset+=16384)
		{
			int size_to_write = 16384;
			//if (hist_size - offset < 16384) size_to_write = hist_size - offset;
			if (hist_size - offset < 16384) size_to_write = (hist_size+40+sizeof(int)) - offset;
			n = write(fd1, &encrypted_buffer[offset], size_to_write);

			//n = write(sockfd, &histogram[offset], size_to_write);
		}
/*
		int answer = 999;
		n = read(sockfd, &answer, sizeof(int));

		if (correct_name && answer) __sync_fetch_and_add(&true_positives,1);
		if (!correct_name && answer) __sync_fetch_and_add(&false_positives,1);
		if (!correct_name && !answer) __sync_fetch_and_add(&true_negatives,1);
		if (correct_name && !answer) __sync_fetch_and_add(&false_negatives,1);
		__sync_fetch_and_add(&g_num_of_requests_answered,1);
*/
	}

	close(fd1);

	close(sockfd);
	free(key);
}


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

	int fd = open("encrypted_dataset.txt", O_RDONLY);
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
	
	//int cnt = load_requests(&req_array, IMAGE_FILES_PATH, pattern2bin);

	memset(&serv_addr, '0', sizeof(serv_addr)); 

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(5006); 

	if(inet_pton(AF_INET, "11.0.0.3", &serv_addr.sin_addr)<=0)
	{
		printf("\n inet_pton error occured\n");
		return 1;
	}

printf("starting therads...\n");

pthread_t tds[NUM_OF_CLIENTS];
for(i=0;i<NUM_OF_CLIENTS;i++) {
	//if(pthread_create(&tds[i], NULL, thread_start, &cnt)) {
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
