#include "Face_Verification_Enclave_t.h"

#include "sgx_trts.h" /* for sgx_ocalloc, sgx_is_outside_enclave */

#include <errno.h>
#include <string.h> /* for memcpy etc */
#include <stdlib.h> /* for malloc/free etc */

#define CHECK_REF_POINTER(ptr, siz) do {	\
	if (!(ptr) || ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define CHECK_UNIQUE_POINTER(ptr, siz) do {	\
	if ((ptr) && ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)


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

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4127)
#pragma warning(disable: 4200)
#endif

static sgx_status_t SGX_CDECL sgx_ecall_train_server(void* pms)
{
	ms_ecall_train_server_t* ms = SGX_CAST(ms_ecall_train_server_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	void* _tmp_arg1 = ms->ms_arg1;
	void* _tmp_arg2 = ms->ms_arg2;

	CHECK_REF_POINTER(pms, sizeof(ms_ecall_train_server_t));

	ms->ms_retval = ecall_train_server(_tmp_arg1, _tmp_arg2);


	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_open_server(void* pms)
{
	ms_ecall_open_server_t* ms = SGX_CAST(ms_ecall_open_server_t*, pms);
	sgx_status_t status = SGX_SUCCESS;

	CHECK_REF_POINTER(pms, sizeof(ms_ecall_open_server_t));

	ms->ms_retval = ecall_open_server();


	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_lib_initialize(void* pms)
{
	ms_ecall_lib_initialize_t* ms = SGX_CAST(ms_ecall_lib_initialize_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	void* _tmp_pool_ptr = ms->ms_pool_ptr;

	CHECK_REF_POINTER(pms, sizeof(ms_ecall_lib_initialize_t));

	ms->ms_retval = ecall_lib_initialize(_tmp_pool_ptr, ms->ms_pool_size);


	return status;
}

SGX_EXTERNC const struct {
	size_t nr_ecall;
	struct {void* call_addr; uint8_t is_priv;} ecall_table[3];
} g_ecall_table = {
	3,
	{
		{(void*)(uintptr_t)sgx_ecall_train_server, 0},
		{(void*)(uintptr_t)sgx_ecall_open_server, 0},
		{(void*)(uintptr_t)sgx_ecall_lib_initialize, 0},
	}
};

SGX_EXTERNC const struct {
	size_t nr_ocall;
	uint8_t entry_table[11][3];
} g_dyn_entry_table = {
	11,
	{
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
	}
};


sgx_status_t SGX_CDECL ocall_readfile(int* retval, char* __buf, size_t __nbytes)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len___buf = __nbytes;

	ms_ocall_readfile_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_readfile_t);
	void *__tmp = NULL;

	ocalloc_size += (__buf != NULL && sgx_is_within_enclave(__buf, _len___buf)) ? _len___buf : 0;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_readfile_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_readfile_t));

	if (__buf != NULL && sgx_is_within_enclave(__buf, _len___buf)) {
		ms->ms___buf = (char*)__tmp;
		__tmp = (void *)((size_t)__tmp + _len___buf);
		memset(ms->ms___buf, 0, _len___buf);
	} else if (__buf == NULL) {
		ms->ms___buf = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	ms->ms___nbytes = __nbytes;
	status = sgx_ocall(0, ms);

	if (retval) *retval = ms->ms_retval;
	if (__buf) memcpy((void*)__buf, ms->ms___buf, _len___buf);

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_FindFirstFile(int* retval)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_ocall_FindFirstFile_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_FindFirstFile_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_FindFirstFile_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_FindFirstFile_t));

	status = sgx_ocall(1, ms);

	if (retval) *retval = ms->ms_retval;

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_FindNextFile(int* retval)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_ocall_FindNextFile_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_FindNextFile_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_FindNextFile_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_FindNextFile_t));

	status = sgx_ocall(2, ms);

	if (retval) *retval = ms->ms_retval;

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_FindClose(int* retval)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_ocall_FindClose_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_FindClose_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_FindClose_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_FindClose_t));

	status = sgx_ocall(3, ms);

	if (retval) *retval = ms->ms_retval;

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_GetKey(unsigned char** retval)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_ocall_GetKey_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_GetKey_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_GetKey_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_GetKey_t));

	status = sgx_ocall(4, ms);

	if (retval) *retval = ms->ms_retval;

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_oc_cpuidex(int cpuinfo[4], int leaf, int subleaf)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_cpuinfo = 4 * sizeof(*cpuinfo);

	ms_sgx_oc_cpuidex_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_oc_cpuidex_t);
	void *__tmp = NULL;

	ocalloc_size += (cpuinfo != NULL && sgx_is_within_enclave(cpuinfo, _len_cpuinfo)) ? _len_cpuinfo : 0;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_oc_cpuidex_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_oc_cpuidex_t));

	if (cpuinfo != NULL && sgx_is_within_enclave(cpuinfo, _len_cpuinfo)) {
		ms->ms_cpuinfo = (int*)__tmp;
		__tmp = (void *)((size_t)__tmp + _len_cpuinfo);
		memcpy(ms->ms_cpuinfo, cpuinfo, _len_cpuinfo);
	} else if (cpuinfo == NULL) {
		ms->ms_cpuinfo = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	ms->ms_leaf = leaf;
	ms->ms_subleaf = subleaf;
	status = sgx_ocall(5, ms);

	if (cpuinfo) memcpy((void*)cpuinfo, ms->ms_cpuinfo, _len_cpuinfo);

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_wait_untrusted_event_ocall(int* retval, const void* self)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_sgx_thread_wait_untrusted_event_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_wait_untrusted_event_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_wait_untrusted_event_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_wait_untrusted_event_ocall_t));

	ms->ms_self = SGX_CAST(void*, self);
	status = sgx_ocall(6, ms);

	if (retval) *retval = ms->ms_retval;

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_set_untrusted_event_ocall(int* retval, const void* waiter)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_sgx_thread_set_untrusted_event_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_set_untrusted_event_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_set_untrusted_event_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_set_untrusted_event_ocall_t));

	ms->ms_waiter = SGX_CAST(void*, waiter);
	status = sgx_ocall(7, ms);

	if (retval) *retval = ms->ms_retval;

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_setwait_untrusted_events_ocall(int* retval, const void* waiter, const void* self)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_sgx_thread_setwait_untrusted_events_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_setwait_untrusted_events_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_setwait_untrusted_events_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_setwait_untrusted_events_ocall_t));

	ms->ms_waiter = SGX_CAST(void*, waiter);
	ms->ms_self = SGX_CAST(void*, self);
	status = sgx_ocall(8, ms);

	if (retval) *retval = ms->ms_retval;

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_set_multiple_untrusted_events_ocall(int* retval, const void** waiters, size_t total)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_waiters = total * sizeof(*waiters);

	ms_sgx_thread_set_multiple_untrusted_events_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_set_multiple_untrusted_events_ocall_t);
	void *__tmp = NULL;

	ocalloc_size += (waiters != NULL && sgx_is_within_enclave(waiters, _len_waiters)) ? _len_waiters : 0;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_set_multiple_untrusted_events_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_set_multiple_untrusted_events_ocall_t));

	if (waiters != NULL && sgx_is_within_enclave(waiters, _len_waiters)) {
		ms->ms_waiters = (void**)__tmp;
		__tmp = (void *)((size_t)__tmp + _len_waiters);
		memcpy((void*)ms->ms_waiters, waiters, _len_waiters);
	} else if (waiters == NULL) {
		ms->ms_waiters = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	ms->ms_total = total;
	status = sgx_ocall(9, ms);

	if (retval) *retval = ms->ms_retval;

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_debug(const char* str)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_str = str ? strlen(str) + 1 : 0;

	ms_ocall_debug_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_debug_t);
	void *__tmp = NULL;

	ocalloc_size += (str != NULL && sgx_is_within_enclave(str, _len_str)) ? _len_str : 0;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_debug_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_debug_t));

	if (str != NULL && sgx_is_within_enclave(str, _len_str)) {
		ms->ms_str = (char*)__tmp;
		__tmp = (void *)((size_t)__tmp + _len_str);
		memcpy((void*)ms->ms_str, str, _len_str);
	} else if (str == NULL) {
		ms->ms_str = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	status = sgx_ocall(10, ms);


	sgx_ocfree();
	return status;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
