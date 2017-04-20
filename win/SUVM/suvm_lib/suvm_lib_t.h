#ifndef SUVM_LIB_T_H__
#define SUVM_LIB_T_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include "sgx_edger8r.h" /* for sgx_ocall etc. */


#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif


int ecall_lib_initialize(void* pool_ptr, size_t pool_size);

sgx_status_t SGX_CDECL ocall_debug(const char* str);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
