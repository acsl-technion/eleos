#ifndef FACE_VERIFICATION_ENCLAVE_U_H__
#define FACE_VERIFICATION_ENCLAVE_U_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include <string.h>
#include "sgx_edger8r.h" /* for sgx_status_t etc. */


#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif

int SGX_UBRIDGE(SGX_NOCONVENTION, ocall_readfile, (char* __buf, size_t __nbytes));
int SGX_UBRIDGE(SGX_NOCONVENTION, ocall_FindFirstFile, ());
int SGX_UBRIDGE(SGX_NOCONVENTION, ocall_FindNextFile, ());
int SGX_UBRIDGE(SGX_NOCONVENTION, ocall_FindClose, ());
unsigned char* SGX_UBRIDGE(SGX_NOCONVENTION, ocall_GetKey, ());
void SGX_UBRIDGE(SGX_CDECL, sgx_oc_cpuidex, (int cpuinfo[4], int leaf, int subleaf));
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_wait_untrusted_event_ocall, (const void* self));
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_set_untrusted_event_ocall, (const void* waiter));
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_setwait_untrusted_events_ocall, (const void* waiter, const void* self));
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_set_multiple_untrusted_events_ocall, (const void** waiters, size_t total));
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_debug, (const char* str));

sgx_status_t ecall_train_server(sgx_enclave_id_t eid, int* retval, void* arg1, void* arg2);
sgx_status_t ecall_open_server(sgx_enclave_id_t eid, int* retval);
sgx_status_t ecall_lib_initialize(sgx_enclave_id_t eid, int* retval, void* pool_ptr, size_t pool_size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
