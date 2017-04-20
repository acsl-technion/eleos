#ifndef FACE_VERIFICATION_ENCLAVE_T_H__
#define FACE_VERIFICATION_ENCLAVE_T_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include "sgx_edger8r.h" /* for sgx_ocall etc. */


#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif


int ecall_train_server(void* arg1, void* arg2);
int ecall_open_server();
int ecall_lib_initialize(void* pool_ptr, size_t pool_size);

sgx_status_t SGX_CDECL ocall_readfile(int* retval, char* __buf, size_t __nbytes);
sgx_status_t SGX_CDECL ocall_FindFirstFile(int* retval);
sgx_status_t SGX_CDECL ocall_FindNextFile(int* retval);
sgx_status_t SGX_CDECL ocall_FindClose(int* retval);
sgx_status_t SGX_CDECL ocall_GetKey(unsigned char** retval);
sgx_status_t SGX_CDECL sgx_oc_cpuidex(int cpuinfo[4], int leaf, int subleaf);
sgx_status_t SGX_CDECL sgx_thread_wait_untrusted_event_ocall(int* retval, const void* self);
sgx_status_t SGX_CDECL sgx_thread_set_untrusted_event_ocall(int* retval, const void* waiter);
sgx_status_t SGX_CDECL sgx_thread_setwait_untrusted_events_ocall(int* retval, const void* waiter, const void* self);
sgx_status_t SGX_CDECL sgx_thread_set_multiple_untrusted_events_ocall(int* retval, const void** waiters, size_t total);
sgx_status_t SGX_CDECL ocall_debug(const char* str);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
