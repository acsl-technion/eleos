#ifndef LIB_SERVICES_T_H__
#define LIB_SERVICES_T_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include "sgx_edger8r.h" /* for sgx_ocall etc. */


#include <stdlib.h> /* for size_t */

#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif


int ecall_lib_initialize(void* pool_ptr, size_t pool_size, void* queue, unsigned char** ptr_to_pin, unsigned long int* size_to_pin, unsigned long long* untrusted_counters);
void ecall_erase_aptr_pcache(int num_entries);
void ecall_background_thread();

sgx_status_t SGX_CDECL ocall_debug(const char* str);
sgx_status_t SGX_CDECL ocall_create_swap_thread();
sgx_status_t SGX_CDECL ocall_create_background_thread();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
