/*
 * rpc_ocall.h
 *
 *  Created on: Aug 11, 2016
 *      Author: user
 */

#ifndef TRUSTEDLIB_LIB_SERVICES_STATIC_TRUSTED_RPC_OCALL_H_
#define TRUSTEDLIB_LIB_SERVICES_STATIC_TRUSTED_RPC_OCALL_H_

#include "../common/Queue.h"

#ifdef __cplusplus
extern "C" {
#endif

sgx_status_t rpc_ocall(int index, void* ms);
int set_rpc_queue(Queue* queue);

#ifdef __cplusplus
}
#endif

#endif /* TRUSTEDLIB_LIB_SERVICES_STATIC_TRUSTED_RPC_OCALL_H_ */
