#include "Face_Verification_Enclave_u.h"
#include <errno.h>

typedef struct ms_ecall_train_server_t {
	int ms_retval;
	void* ms_arg1;
	void* ms_arg2;
} ms_ecall_train_server_t;

typedef struct ms_ecall_open_server_t {
	int ms_retval;
} ms_ecall_open_server_t;

typedef struct ms_ecall_lib_initialize_t {
	int ms_retval;
	void* ms_pool_ptr;
	size_t ms_pool_size;
} ms_ecall_lib_initialize_t;

typedef struct ms_ocall_readfile_t {
	int ms_retval;
	char* ms___buf;
	size_t ms___nbytes;
} ms_ocall_readfile_t;

typedef struct ms_ocall_FindFirstFile_t {
	int ms_retval;
} ms_ocall_FindFirstFile_t;

typedef struct ms_ocall_FindNextFile_t {
	int ms_retval;
} ms_ocall_FindNextFile_t;

typedef struct ms_ocall_FindClose_t {
	int ms_retval;
} ms_ocall_FindClose_t;

typedef struct ms_ocall_GetKey_t {
	unsigned char* ms_retval;
} ms_ocall_GetKey_t;

typedef struct ms_sgx_oc_cpuidex_t {
	int* ms_cpuinfo;
	int ms_leaf;
	int ms_subleaf;
} ms_sgx_oc_cpuidex_t;

typedef struct ms_sgx_thread_wait_untrusted_event_ocall_t {
	int ms_retval;
	void* ms_self;
} ms_sgx_thread_wait_untrusted_event_ocall_t;

typedef struct ms_sgx_thread_set_untrusted_event_ocall_t {
	int ms_retval;
	void* ms_waiter;
} ms_sgx_thread_set_untrusted_event_ocall_t;

typedef struct ms_sgx_thread_setwait_untrusted_events_ocall_t {
	int ms_retval;
	void* ms_waiter;
	void* ms_self;
} ms_sgx_thread_setwait_untrusted_events_ocall_t;

typedef struct ms_sgx_thread_set_multiple_untrusted_events_ocall_t {
	int ms_retval;
	void** ms_waiters;
	size_t ms_total;
} ms_sgx_thread_set_multiple_untrusted_events_ocall_t;

typedef struct ms_ocall_debug_t {
	char* ms_str;
} ms_ocall_debug_t;

static sgx_status_t SGX_CDECL Face_Verification_Enclave_ocall_readfile(void* pms)
{
	ms_ocall_readfile_t* ms = SGX_CAST(ms_ocall_readfile_t*, pms);
	ms->ms_retval = ocall_readfile(ms->ms___buf, ms->ms___nbytes);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Face_Verification_Enclave_ocall_FindFirstFile(void* pms)
{
	ms_ocall_FindFirstFile_t* ms = SGX_CAST(ms_ocall_FindFirstFile_t*, pms);
	ms->ms_retval = ocall_FindFirstFile();

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Face_Verification_Enclave_ocall_FindNextFile(void* pms)
{
	ms_ocall_FindNextFile_t* ms = SGX_CAST(ms_ocall_FindNextFile_t*, pms);
	ms->ms_retval = ocall_FindNextFile();

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Face_Verification_Enclave_ocall_FindClose(void* pms)
{
	ms_ocall_FindClose_t* ms = SGX_CAST(ms_ocall_FindClose_t*, pms);
	ms->ms_retval = ocall_FindClose();

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Face_Verification_Enclave_ocall_GetKey(void* pms)
{
	ms_ocall_GetKey_t* ms = SGX_CAST(ms_ocall_GetKey_t*, pms);
	ms->ms_retval = ocall_GetKey();

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Face_Verification_Enclave_sgx_oc_cpuidex(void* pms)
{
	ms_sgx_oc_cpuidex_t* ms = SGX_CAST(ms_sgx_oc_cpuidex_t*, pms);
	sgx_oc_cpuidex(ms->ms_cpuinfo, ms->ms_leaf, ms->ms_subleaf);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Face_Verification_Enclave_sgx_thread_wait_untrusted_event_ocall(void* pms)
{
	ms_sgx_thread_wait_untrusted_event_ocall_t* ms = SGX_CAST(ms_sgx_thread_wait_untrusted_event_ocall_t*, pms);
	ms->ms_retval = sgx_thread_wait_untrusted_event_ocall((const void*)ms->ms_self);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Face_Verification_Enclave_sgx_thread_set_untrusted_event_ocall(void* pms)
{
	ms_sgx_thread_set_untrusted_event_ocall_t* ms = SGX_CAST(ms_sgx_thread_set_untrusted_event_ocall_t*, pms);
	ms->ms_retval = sgx_thread_set_untrusted_event_ocall((const void*)ms->ms_waiter);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Face_Verification_Enclave_sgx_thread_setwait_untrusted_events_ocall(void* pms)
{
	ms_sgx_thread_setwait_untrusted_events_ocall_t* ms = SGX_CAST(ms_sgx_thread_setwait_untrusted_events_ocall_t*, pms);
	ms->ms_retval = sgx_thread_setwait_untrusted_events_ocall((const void*)ms->ms_waiter, (const void*)ms->ms_self);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Face_Verification_Enclave_sgx_thread_set_multiple_untrusted_events_ocall(void* pms)
{
	ms_sgx_thread_set_multiple_untrusted_events_ocall_t* ms = SGX_CAST(ms_sgx_thread_set_multiple_untrusted_events_ocall_t*, pms);
	ms->ms_retval = sgx_thread_set_multiple_untrusted_events_ocall((const void**)ms->ms_waiters, ms->ms_total);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Face_Verification_Enclave_ocall_debug(void* pms)
{
	ms_ocall_debug_t* ms = SGX_CAST(ms_ocall_debug_t*, pms);
	ocall_debug((const char*)ms->ms_str);

	return SGX_SUCCESS;
}

static const struct {
	size_t nr_ocall;
	void * func_addr[11];
} ocall_table_Face_Verification_Enclave = {
	11,
	{
		(void*)(uintptr_t)Face_Verification_Enclave_ocall_readfile,
		(void*)(uintptr_t)Face_Verification_Enclave_ocall_FindFirstFile,
		(void*)(uintptr_t)Face_Verification_Enclave_ocall_FindNextFile,
		(void*)(uintptr_t)Face_Verification_Enclave_ocall_FindClose,
		(void*)(uintptr_t)Face_Verification_Enclave_ocall_GetKey,
		(void*)(uintptr_t)Face_Verification_Enclave_sgx_oc_cpuidex,
		(void*)(uintptr_t)Face_Verification_Enclave_sgx_thread_wait_untrusted_event_ocall,
		(void*)(uintptr_t)Face_Verification_Enclave_sgx_thread_set_untrusted_event_ocall,
		(void*)(uintptr_t)Face_Verification_Enclave_sgx_thread_setwait_untrusted_events_ocall,
		(void*)(uintptr_t)Face_Verification_Enclave_sgx_thread_set_multiple_untrusted_events_ocall,
		(void*)(uintptr_t)Face_Verification_Enclave_ocall_debug,
	}
};

sgx_status_t ecall_train_server(sgx_enclave_id_t eid, int* retval, void* arg1, void* arg2)
{
	sgx_status_t status;
	ms_ecall_train_server_t ms;
	ms.ms_arg1 = arg1;
	ms.ms_arg2 = arg2;
	status = sgx_ecall(eid, 0, &ocall_table_Face_Verification_Enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t ecall_open_server(sgx_enclave_id_t eid, int* retval)
{
	sgx_status_t status;
	ms_ecall_open_server_t ms;
	status = sgx_ecall(eid, 1, &ocall_table_Face_Verification_Enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t ecall_lib_initialize(sgx_enclave_id_t eid, int* retval, void* pool_ptr, size_t pool_size)
{
	sgx_status_t status;
	ms_ecall_lib_initialize_t ms;
	ms.ms_pool_ptr = pool_ptr;
	ms.ms_pool_size = pool_size;
	status = sgx_ecall(eid, 2, &ocall_table_Face_Verification_Enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

