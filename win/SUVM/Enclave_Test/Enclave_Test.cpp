#include "Enclave_Test_t.h"
#include "sgx_trts.h"

#include <Aptr.h>
#include <stdlib.h>
#include <mem.h>
#include <sgx_trts.h>

#define BUF_SIZE 500 * 1024 * 1024 // 500 MB

int ecall_test_sgx()
{
	int* g_buffer = (int*)malloc(BUF_SIZE);
	ASSERT(!g_buffer);

	return 0;
}

int ecall_test_suvm()
{
	// writing to a large buffer with aptrs
	int* g_aptr_buffer = (int*)memsys5Malloc(BUF_SIZE);
	ASSERT(g_aptr_buffer);

	Aptr<int> ap(g_aptr_buffer, BUF_SIZE, 0);
	ap.m_aptr.hint_write = 1;
	for (int i = 0; i < BUF_SIZE / sizeof(int); i++) {
		unsigned int* val = (unsigned int*)deref(&ap.m_aptr, ap.m_base_page_index);
		*val = i;
		MOVE_APTR(&ap.m_aptr, sizeof(int));
	}

	Aptr<int> ap_new(g_aptr_buffer, BUF_SIZE, 0);
	// reading those values with aptrs
	ap_new.m_aptr.hint_write = 0;
	for (int i = 0; i < BUF_SIZE / sizeof(int); i++) {
		unsigned int* val = (unsigned int*)deref(&ap_new.m_aptr, ap_new.m_base_page_index);
		ASSERT(*val == i);
		//debug("expected %d, actual %d\n", i, *val);
		MOVE_APTR(&ap_new.m_aptr, sizeof(int));
	}

	return 0;
}
