#include "lib_services_t.h"  /* print_string */
#include "../common/Queue.h"
#include "Aptr.h"
#include "rpc_ocall.h"
#include "lib_services_t.h"

int ecall_lib_initialize(void* pool_ptr, size_t pool_size, void* queue, unsigned char** ptr_to_pin, unsigned long* size_to_pin, unsigned long long* untrusted_counters)
{
	if (pool_ptr == NULL || queue == NULL)
	{
		return -1;
	}

	int result = set_rpc_queue((Queue*)queue);
	result &= initialize_aptr(pool_ptr, pool_size, ptr_to_pin, size_to_pin,untrusted_counters);

	return result;
}

void ecall_erase_aptr_pcache(int num_entries)
{
	remove_entries_pcache(num_entries);
}
