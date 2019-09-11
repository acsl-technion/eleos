/*
 * communication.h
 *
 *  Created on: Jun 25, 2016
 *      Author: user
 */

#ifndef TRUSTEDLIB_LIB_SERVICES_UNTRUSTED_INIT_SERVICES_H_
#define TRUSTEDLIB_LIB_SERVICES_UNTRUSTED_INIT_SERVICES_H_

#ifdef __cplusplus
extern "C" {
#endif

int initialize_lib(sgx_enclave_id_t eid, bool start_rpc_thread);
int cleanup_lib(sgx_enclave_id_t eid);
void ocall_create_swap_thread();
void* get_pool_ptr_debug();
void* swap_thread(void* arg);
void ocall_debug(const char *str);
void ocall_create_background_thread();
void ocall_pthread_destroy();
#ifdef __cplusplus
}
#endif

#endif /* TRUSTEDLIB_LIB_SERVICES_UNTRUSTED_INIT_SERVICES_H_ */
