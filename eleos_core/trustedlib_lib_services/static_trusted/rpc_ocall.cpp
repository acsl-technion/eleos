/*
 * rpc_ocall.cpp
 *
 *  Created on: Aug 11, 2016
 *      Author: user
 */

#include "mem.h"
#include <sgx_trts.h>
#include "../common/Queue.h"
#include "rpc_ocall.h"
#include "../common/SyncUtils.h"
#include "Aptr.h"
Queue* rpc_queue;

int set_rpc_queue(Queue* queue)
{
	rpc_queue = queue;
	return 0;
}

sgx_status_t rpc_ocall(int index, void* ms)
{
	// allocate in non-protected RAM
	request* req = (request*)memsys5Malloc(sizeof(request));

	if (req == NULL)
	{
		return SGX_ERROR_OCALL_NOT_ALLOWED;
	}

	req->ocall_index = index;
	req->buffer = ms;
	req->is_done = 1;
	// Note: the consumer thread will unlock, we will wait on this lock until it does
	rpc_queue->enqueue(req);
	spin_lock(&req->is_done);

	memsys5Free(req);

	return SGX_SUCCESS;
}
