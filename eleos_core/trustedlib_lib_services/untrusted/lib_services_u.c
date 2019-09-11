#include "lib_services_u.h"
#include <errno.h>

typedef struct ms_ecall_lib_initialize_t {
	int ms_retval;
	void* ms_pool_ptr;
	size_t ms_pool_size;
	void* ms_queue;
	unsigned char** ms_ptr_to_pin;
	unsigned long int* ms_size_to_pin;
	unsigned long long* ms_untrusted_counters;
} ms_ecall_lib_initialize_t;

typedef struct ms_ecall_erase_aptr_pcache_t {
	int ms_num_entries;
} ms_ecall_erase_aptr_pcache_t;


typedef struct ms_ocall_debug_t {
	char* ms_str;
} ms_ocall_debug_t;



static sgx_status_t SGX_CDECL lib_services_ocall_debug(void* pms)
{
	ms_ocall_debug_t* ms = SGX_CAST(ms_ocall_debug_t*, pms);
	ocall_debug((const char*)ms->ms_str);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL lib_services_ocall_create_swap_thread(void* pms)
{
	if (pms != NULL) return SGX_ERROR_INVALID_PARAMETER;
	ocall_create_swap_thread();
	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL lib_services_ocall_create_background_thread(void* pms)
{
	if (pms != NULL) return SGX_ERROR_INVALID_PARAMETER;
	ocall_create_background_thread();
	return SGX_SUCCESS;
}

static const struct {
	size_t nr_ocall;
	void * table[3];
} ocall_table_lib_services = {
	3,
	{
		(void*)lib_services_ocall_debug,
		(void*)lib_services_ocall_create_swap_thread,
		(void*)lib_services_ocall_create_background_thread,
	}
};
sgx_status_t ecall_lib_initialize(sgx_enclave_id_t eid, int* retval, void* pool_ptr, size_t pool_size, void* queue, unsigned char** ptr_to_pin, unsigned long int* size_to_pin, unsigned long long* untrusted_counters)
{
	sgx_status_t status;
	ms_ecall_lib_initialize_t ms;
	ms.ms_pool_ptr = pool_ptr;
	ms.ms_pool_size = pool_size;
	ms.ms_queue = queue;
	ms.ms_ptr_to_pin = ptr_to_pin;
	ms.ms_size_to_pin = size_to_pin;
	ms.ms_untrusted_counters = untrusted_counters;
	status = sgx_ecall(eid, 0, &ocall_table_lib_services, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t ecall_erase_aptr_pcache(sgx_enclave_id_t eid, int num_entries)
{
	sgx_status_t status;
	ms_ecall_erase_aptr_pcache_t ms;
	ms.ms_num_entries = num_entries;
	status = sgx_ecall(eid, 1, &ocall_table_lib_services, &ms);
	return status;
}

sgx_status_t ecall_background_thread(sgx_enclave_id_t eid)
{
	sgx_status_t status;
	status = sgx_ecall(eid, 2, &ocall_table_lib_services, NULL);
	return status;
}

