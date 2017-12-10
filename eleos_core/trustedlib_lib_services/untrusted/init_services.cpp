/*
 * communication.cpp
 *
 *  Created on: Jun 24, 2016
 *      Author: user
 */

#include <sys/ioctl.h>
#include <pthread.h>                                                         
#include <signal.h>  
#include "../common/request.h"
#include "../common/Queue.h"
#include "lib_services_u.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <malloc.h>

#include <assert.h>

#include "init_services.h"
#include "../common/SyncUtils.h"
#include "rpc_worker.h"
#include "../common/PerfCounters.h"

void* pool_ptr;
void** arr;
int swap_eid;
pthread_t  tid =4000;

unsigned long long* g_perf_counters;

void* swap_thread(void* arg)
{
	while (1)
	{
		int ret;
		pthread_mutex_lock(&g_aptr_mutex);
		while (g_swap_thread_work == false)
		{
			// wait on condition variable until its ok to work
			ret = pthread_cond_wait(&g_aptr_cond_var, &g_aptr_mutex);
		}

		g_swap_thread_work = false;
		pthread_mutex_unlock(&g_aptr_mutex);

		int entries_to_remove = -1; // -1 = Remove LOAD_FACTOR entries if the cache is currently heavily loaded
		ecall_erase_aptr_pcache(swap_eid, entries_to_remove);
	}
	return NULL;
}

void ocall_debug(const char *str)
{
	printf("%s",str);
}

void ocall_create_swap_thread()
{
	pthread_t  t;
	int res = pthread_create(&t, NULL, swap_thread, NULL);

	if (res)
	{
		exit (-1);
	}

}

void* background_thread_me(void* arg)
{
	ecall_background_thread(swap_eid);
	return NULL;
}

void ocall_create_background_thread(){
	int res = pthread_create(&tid, NULL, background_thread_me, NULL);
	
	//pthread_detach(tid);
	
	//tid++;
	if (res)
	{	printf("failed to make thread\n");
		exit (-1);
	}
}

void ocall_pthread_destroy(){
	printf("Gonna kill prefetch thread...\n");	
	int res = pthread_kill( tid, SIGKILL);
	if (res<0)
	{	printf("failed to kill thread\n");
		exit (-1);
	}
}
void* get_pool_ptr_debug()
{
	return pool_ptr;
}


int initialize_lib(sgx_enclave_id_t eid, bool start_rpc_thread)
{
	swap_eid = eid;
	size_t pool_size = 2l *(1l << 30l);			// 2G backing store

	pool_ptr = malloc(pool_size);
	if (pool_ptr == NULL || pool_ptr == MAP_FAILED)
	{
		printf("failed allocating pool\n");
		abort();
	}

	g_swap_thread_work = false;
	int ret = pthread_cond_init(&g_aptr_cond_var, NULL);
	ret = pthread_mutex_init(&g_aptr_mutex, NULL);
	void* rpc_queue = start_rpc_worker(start_rpc_thread);

	int retval;
	unsigned char* ptr_to_pin = NULL;
	unsigned long size_to_pin = 0;
	g_perf_counters=(unsigned long long*)malloc(sizeof(unsigned long long)*NUM_COUNTERS);
	CLEAN_COUNTERS(g_perf_counters);
	sgx_status_t status = ecall_lib_initialize(eid, &retval, pool_ptr, pool_size, rpc_queue, &ptr_to_pin, &size_to_pin,g_perf_counters);

	if (status != SGX_SUCCESS)
	{
		printf("failed to initialize\n");
		return -1;
	}

	if(retval)
		printf("retval = %d\n", retval);

	return retval;
}

int cleanup_lib(sgx_enclave_id_t eid)
{
	close_rpc_worker();

	free(arr);
	free(pool_ptr);
	ocall_pthread_destroy();
	return 0;
}
