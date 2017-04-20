/*
 * communication.h
 *
 *  Created on: Jun 25, 2016
 *      Author: user
 */

#ifndef TRUSTEDLIB_LIB_SERVICES_UNTRUSTED_INIT_SERVICES_H_
#define TRUSTEDLIB_LIB_SERVICES_UNTRUSTED_INIT_SERVICES_H_

int initialize_lib(sgx_enclave_id_t eid, bool start_rpc_thread);
int cleanup_lib(sgx_enclave_id_t eid);

void* get_pool_ptr_debug();

#endif /* TRUSTEDLIB_LIB_SERVICES_UNTRUSTED_INIT_SERVICES_H_ */
