#ifndef LIB_SERVICES_U_H__
#define LIB_SERVICES_U_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include <string.h>
#include "sgx_edger8r.h" /* for sgx_satus_t etc. */


#include <stdlib.h> /* for size_t */

#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif

void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_debug, (const char* str));
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_create_swap_thread, ());
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_create_background_thread, ());

sgx_status_t ecall_lib_initialize(sgx_enclave_id_t eid, int* retval, void* pool_ptr, size_t pool_size, void* queue, unsigned char** ptr_to_pin, unsigned long int* size_to_pin, unsigned long long* untrusted_counters);
sgx_status_t ecall_erase_aptr_pcache(sgx_enclave_id_t eid, int num_entries);
sgx_status_t ecall_background_thread(sgx_enclave_id_t eid);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
