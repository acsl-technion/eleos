/*
 * rpc_worker.cpp
 *
 *  Created on: Aug 11, 2016
 *      Author: user
 */

#include "rpc_worker.h"
#include "../common/SyncUtils.h"
#include "../common/Queue.h"
#include <pthread.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

ocall_table_template* ocall_table;
volatile int exit_request;
Queue* rpc_queue;

pthread_cond_t g_aptr_cond_var;
pthread_mutex_t g_aptr_mutex;
bool g_swap_thread_work;

void set_ocall_table(const void* _ocall_table)
{
	if (ocall_table == NULL)
	{
		ocall_table = (ocall_table_template*) _ocall_table;
	}
}

int do_work(request* req)
{
	if (req->ocall_index == -1)
	{
		// special call to async clean aptrs
		pthread_mutex_lock(&g_aptr_mutex);
		g_swap_thread_work = true;
		pthread_cond_signal(&g_aptr_cond_var);
		pthread_mutex_unlock(&g_aptr_mutex);

		// async, so no need for new request
		spin_unlock(&req->is_done);
		return 0;
	}

	bridge_fn_t bridge = reinterpret_cast<bridge_fn_t>((&ocall_table->table)[req->ocall_index]);
	int error = bridge(req->buffer);
	assert (ret == SGX_SUCCESS);
	spin_unlock(&req->is_done);

	return 0;
}

void* rpc_thread(void* state)
{
	Queue* queue = (Queue*)state;

	while (true)
	{
		if (exit_request)
		{
			return 0;
		}

		request* req = queue->dequeue();

		if (req == NULL)
		{
			__asm__("pause");
		}
		else
		{
			do_work(req);
		}
	}

	return 0;
}

void* start_rpc_worker(bool start_rpc_thread)
{
	rpc_queue = new Queue();
	exit_request = false;
	if (start_rpc_thread)
	{
		pthread_t communication_thread;
		int res = pthread_create(&communication_thread, NULL, rpc_thread, rpc_queue);

		if (res)
		{
			delete rpc_queue;
			return NULL;
		}
	}

	return rpc_queue;
}

int close_rpc_worker()
{
	exit_request = true;
	delete rpc_queue;

	return 0;
}
