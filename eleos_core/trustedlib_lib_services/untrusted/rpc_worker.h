/*
 * rpc_worker.h
 *
 *  Created on: Aug 16, 2016
 *      Author: user
 */

#ifndef TRUSTEDLIB_LIB_SERVICES_UNTRUSTED_RPC_WORKER_H_
#define TRUSTEDLIB_LIB_SERVICES_UNTRUSTED_RPC_WORKER_H_

#include <stdlib.h>
#include <stdbool.h>

extern pthread_cond_t g_aptr_cond_var;
extern pthread_mutex_t g_aptr_mutex;
extern bool g_swap_thread_work;

//typedef sgx_status_t (*bridge_func)(void*);
typedef int (*bridge_fn_t)(const void*);

struct ocall_table_template
{
	size_t nr_ocall;
	void* table;
};

#ifdef __cplusplus
extern "C" {
#endif

void set_ocall_table(const void* _ocall_table);
void* start_rpc_worker(bool start_rpc_thread);
int close_rpc_worker();

#ifdef __cplusplus
}
#endif

#endif /* TRUSTEDLIB_LIB_SERVICES_UNTRUSTED_RPC_WORKER_H_ */
