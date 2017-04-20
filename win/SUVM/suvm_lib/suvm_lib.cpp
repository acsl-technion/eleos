#include "suvm_lib_t.h"
#include "sgx_trts.h"

#include "Aptr.h"

int ecall_lib_initialize(void* pool_ptr, size_t pool_size)
{
	if (pool_ptr == NULL)
	{
		return -1;
	}

	int result = initialize_aptr(pool_ptr, pool_size);

	return result;
}