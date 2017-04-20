/*
 * Aptr.cpp
 *
 *  Created on: Jul 17, 2016
 *      Author: user
 */
#include "PerfCounters.h"
#include "Aptr.h"
#include <sgx_trts.h>
#include "mem.h"
#include <stdlib.h>

#include <stdarg.h>
#include <string.h>
#include <stdio.h>      /* vsnprintf */
#include <stlport/list>

unsigned char DUMMY = 0;
/*
void debug(const char *fmt, ...)
{
	char buf[BUFSIZ] = {'\0'};
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZ, fmt, ap);
	va_end(ap);
	ocall_debug(buf);
}
*/

sgx_aes_gcm_128bit_key_t g_eviction_key;
unsigned char* g_base_pool_ptr;
size_t g_pool_size;
bool g_is_initialized;

// input: pgae index
// output: trusted memory
PageCache g_aptr_crypto_page_cache_l1;
std::vector<unsigned char**> g_free_epc_pages;

// input: page index
// output: crypto (i.e. MAC, nonse)
CryptoCache g_aptr_crypto_page_cache_llc;

volatile unsigned int g_free_epc_pages_lock;
unsigned char* g_slab;


int cleanup_resources()
{
	free(g_slab);
	/*
	for (auto it = g_free_epc_pages.begin(); it < g_free_epc_pages.end(); it++)
	{
		unsigned char** epc_page = *it;
		free(*epc_page);
	}
	*/

	// clean data structures unused stack memory
	g_free_epc_pages.clear();
	g_aptr_crypto_page_cache_l1.cleanup();
	g_aptr_crypto_page_cache_llc.cleanup();

	return 0;
}

void remove_entries_pcache(int num_entries)
{
	/*
		int cache_size = CACHE_CAPACITY - LOAD_FACTOR + num_entries;

		if (num_entries != -1)
		{
			// special case: worker was called just to reduce pressure on the cache
			spin_lock(&g_allocated_pages_lock);
			cache_size = g_allocated_pages;
			spin_unlock(&g_allocated_pages_lock);
		}

		while (cache_size >= CACHE_CAPACITY - LOAD_FACTOR)
		{
			int page_to_evict;
			unsigned char** prm_addr_to_evict;
			run_clock(&page_to_evict, &prm_addr_to_evict);

			spin_lock(&g_page_ref_count_arr[page_to_evict].lock);

			if (g_page_ref_count_arr[page_to_evict].ref_count == 0)
			{
				try_evict_page(page_to_evict, prm_addr_to_evict);
			}

			spin_unlock(&g_page_ref_count_arr[page_to_evict].lock);

			cache_size--;
		}
	*/
}

int create_swap_thread()
{
	//ocall_create_swap_thread();

	return 0;
}

// assume init is thread-unsafe - under user responsibility.
int initialize_aptr(void* ptr_pool, // untrusted memory ptr
	size_t _pool_size)
{
	if (g_is_initialized || ptr_pool == NULL)
	{
		ASSERT(false); // error
	}

	DUMMY = 0;

	g_base_pool_ptr = (unsigned char*)ptr_pool;
	g_pool_size = _pool_size;

	g_aptr_crypto_page_cache_l1.init(/*num_of_buckets=*/CACHE_CAPACITY * 10,/*backing_store_size=*/_pool_size / PAGE_SIZE, /*page cache size=*/ CACHE_CAPACITY);
	g_aptr_crypto_page_cache_llc.init(LLC_CAPACITY);

	unsigned char** epc_arr = (unsigned char**)malloc(CACHE_CAPACITY * sizeof(void*));
	g_slab = (unsigned char*)malloc(PAGE_SIZE*CACHE_CAPACITY + 0x1000l);
	g_slab = (unsigned char*)((uint64_t)g_slab & ~0xFFF);

	for (size_t i = 0; i < CACHE_CAPACITY; i++)
	{
		epc_arr[i] = g_slab + i*PAGE_SIZE; //(unsigned char*)malloc(PAGE_SIZE);
		if (epc_arr[i] == NULL)
		{
			ASSERT(false); // not enough epc space
		}

		g_free_epc_pages.push_back(&epc_arr[i]);
	}

	int rc = memsys5Init(NULL, ptr_pool, _pool_size, MN_REQ);
	if (rc != 0)
	{
		ASSERT(false); // error
	}

	sgx_read_rand(g_eviction_key, sizeof(sgx_aes_gcm_128bit_key_t));

	if (create_swap_thread() != 0)
	{
		debug("create swap thread failed\n");
		ASSERT(false);
		return -1;
	}

	g_is_initialized = true;

	return 0;
}

unsigned long g_evict_counter = 0, g_decrypt_counter = 0;
unsigned long g_dirty = 0;

int try_evict_page(item_t* pce)
{
	int page_index = pce->page_index;
	unsigned char** prm_addr = pce->epc;
	long long prm_addr_long = (long long)*prm_addr;

	if ((prm_addr_long & 0x01) == 1)
	{
		prm_addr_long &= 0xFFFFFFFFFFFFFFFE;
		*prm_addr = (unsigned char*)prm_addr_long;
		// This either adds or not internally, more importantly it returns a linked struct in the crypto cache data structure. 
		auto evicted_pagemetadata = g_aptr_crypto_page_cache_llc.add(page_index);

		sgx_aes_gcm_128bit_tag_t& mac = evicted_pagemetadata->mac_ptr[0];
		sgx_read_rand(evicted_pagemetadata->nonce, NONCE_BYTE_SIZE);
		unsigned char* ram_page_ptr = g_base_pool_ptr + page_index * PAGE_SIZE;

		//BEGIN_PF_MEASURE(g_perf_counters); // cccc
		sgx_status_t ret = sgx_rijndael128GCM_encrypt(&g_eviction_key,
			*prm_addr,
			PAGE_SIZE,
			ram_page_ptr,
			evicted_pagemetadata->nonce,
			NONCE_BYTE_SIZE,
			NULL,
			0,
			&mac);

		ASSERT(ret == SGX_SUCCESS);
		//END_PF_MEASURE(g_perf_counters); // cccc

		//ENCRYPT_COUNT_INC(g_perf_counters);

		//evicted_pagemetadata->is_initialized[0] = true;
	}

	g_aptr_crypto_page_cache_l1.remove(page_index);
	spin_lock(&g_free_epc_pages_lock);
	g_free_epc_pages.push_back(prm_addr);
	spin_unlock(&g_free_epc_pages_lock);
	// written back encrypted data - done
	return 0;
}


void page_fault(aptr_t* aptr, int base_page_index)
{
	int tmp_page = (aptr->offset) >> PAGE_SIZE_LOG;
	// first remove ref count from aptr only if it was linked before...
	if (*aptr->prm_ptr != NULL)
	{
		int prev_page_index = base_page_index + (aptr->page);
#ifdef APTR_RANDOM_ACCESS
		if (tmp_page != 0) // moved page and not sub page
		{
			g_aptr_crypto_page_cache_l1.dec_ref_count(prev_page_index);
		}
#else
		g_aptr_crypto_page_cache_l1.dec_ref_count(prev_page_index);
#endif
	}

	// now update aptr to its new state.
	int tmp_offset = (aptr->offset)&PAGE_BITS_MASK;
	int real_page = aptr->page + tmp_page;
	// now deal with the page fault.

	int page_index = base_page_index + real_page;
	aptr->offset = tmp_offset;
	aptr->page = real_page;

#ifdef APTR_RANDOM_ACCESS
	int sub_page_index = aptr->offset >> SUB_PAGE_SIZE_LOG;
	//sub_page_index %= SUB_PAGE_COUNT;
	aptr->sub_page_index = sub_page_index;
	if (tmp_page != 0) // actually changed the page index
	{
		g_aptr_crypto_page_cache_l1.inc_ref_count(page_index);
	}
#else
	int sub_page_index = 0; // No O_DIRECT, sub_page is by default 0.
		// This ensures that this page cannot be evicted from the page cache, 
		// MUST DO IT BEFORE FIGURING OUT MINOR/MAJOR PF
	g_aptr_crypto_page_cache_l1.inc_ref_count(page_index);
#endif

	item_t* it = g_aptr_crypto_page_cache_l1.get(page_index);
#ifdef APTR_RANDOM_ACCESS
	bool is_minor = it != NULL && it->is_valid[sub_page_index];
#else
	bool is_minor = it != NULL;
#endif
	if (is_minor) // minor #PF
	{
		//COUNTER_2_INC_5(g_perf_counters); // to make sure I miss this one
		aptr->prm_ptr = it->epc;
		return;
	}

	if (g_aptr_crypto_page_cache_l1.m_page_cache_size >= CACHE_CAPACITY - 10) // dirty hack for free pages, assuming only up to 10 threads in the system
	{
		/*
		if (cache_size >= CACHE_CAPACITY - LOAD_FACTOR)
		{
			//start async worker
			rpc_ocall(-1, NULL);
		}
		*/
		item_t* page_to_evict = g_aptr_crypto_page_cache_l1.get_page_index_to_evict();
		try_evict_page(page_to_evict);
	}

	spin_lock(&g_free_epc_pages_lock);
	auto free_epc_page_it = g_free_epc_pages.begin();
	ASSERT(free_epc_page_it != g_free_epc_pages.end());
	//unsigned char** prm_ptr = *free_epc_page_it; 
	unsigned char** prm_ptr = g_free_epc_pages.back();
	g_free_epc_pages.pop_back();
	//g_free_epc_pages.erase(free_epc_page_it); // used - remove from list
	spin_unlock(&g_free_epc_pages_lock);

	auto crypto_item = g_aptr_crypto_page_cache_llc.get(page_index);
#ifdef APTR_RANDOM_ACCESS
	if (crypto_item != NULL && crypto_item->is_initialized[sub_page_index])
#else
	if (crypto_item != NULL) // not in crypto cache
#endif
	{
		unsigned char* ram_page_ptr = g_base_pool_ptr + page_index * PAGE_SIZE + sub_page_index * SUB_PAGE_SIZE;
		unsigned char* epc_ptr = (uint8_t*)*prm_ptr + sub_page_index * SUB_PAGE_SIZE;
		unsigned char* nonce = crypto_item->nonce + sub_page_index * NONCE_BYTE_SIZE;
		sgx_aes_gcm_128bit_tag_t& mac = crypto_item->mac_ptr[sub_page_index];

		sgx_status_t ret = sgx_rijndael128GCM_decrypt(&g_eviction_key,
			ram_page_ptr,
			SUB_PAGE_SIZE,
			epc_ptr,
			nonce,
			NONCE_BYTE_SIZE,
			NULL,
			0,
			&mac);

		ASSERT(ret == SGX_SUCCESS);

	}
	else {
		//COUNTER_2_INC_5(g_perf_counters); // to make sure I miss this one
	}

	//PF_COUNT_INC(g_perf_counters);

	// Try add to cache, if other aptr already added while we worked on it - just return it as a minor, and return our page to the free pages pool.
#ifdef APTR_RANDOM_ACCESS
	if (!g_aptr_crypto_page_cache_l1.try_add(page_index, prm_ptr, sub_page_index))
#else
	if (!g_aptr_crypto_page_cache_l1.try_add(page_index, prm_ptr))
#endif
	{
		item_t* found = g_aptr_crypto_page_cache_l1.get(page_index);
		ASSERT(found != NULL); // if NULL - abort! 

#ifdef APTR_RANDOM_ACCESS
		if (!found->is_valid[sub_page_index])
		{
			memcpy(*found->epc + sub_page_index * SUB_PAGE_SIZE, (uint8_t*)*prm_ptr + sub_page_index * SUB_PAGE_SIZE, SUB_PAGE_SIZE);
			found->is_valid[sub_page_index] = true;
		}
#endif
		spin_lock(&g_free_epc_pages_lock);
		g_free_epc_pages.push_back(prm_ptr);
		spin_unlock(&g_free_epc_pages_lock);
		prm_ptr = found->epc;
	}

	// finally, link
	aptr->prm_ptr = prm_ptr;
}

#ifdef APTR_RANDOM_ACCESS

int aptr_fsync(aptr_t* aptr, int base_page_index)
{
	if (*aptr->prm_ptr == NULL)
	{
		// nothing to sync
		return -1;
	}

	int page_index = base_page_index + aptr->page;

	if (aptr->hint_write)
	{
		auto evicted_pagemetadata = g_aptr_crypto_page_cache_llc.add(page_index);
		int sub_page_index = aptr->sub_page_index;
		unsigned char* ram_page_ptr = g_base_pool_ptr + page_index * PAGE_SIZE + sub_page_index * SUB_PAGE_SIZE;
		sgx_aes_gcm_128bit_tag_t& mac = evicted_pagemetadata->mac_ptr[sub_page_index];
		unsigned char* nonce = evicted_pagemetadata->nonce + sub_page_index * NONCE_BYTE_SIZE;
		sgx_read_rand(nonce, NONCE_BYTE_SIZE);
		unsigned char* epc_ptr = *aptr->prm_ptr + sub_page_index * SUB_PAGE_SIZE;

		sgx_status_t ret = sgx_rijndael128GCM_encrypt(&g_eviction_key,
			epc_ptr,
			SUB_PAGE_SIZE,
			ram_page_ptr,
			nonce,
			NONCE_BYTE_SIZE,
			NULL,
			0,
			&mac);

		ASSERT(ret == SGX_SUCCESS);

		// keep track of free'd addresses
		evicted_pagemetadata->is_initialized[sub_page_index] = true;
	}

	if (!g_aptr_crypto_page_cache_l1.try_remove(page_index))
	{
		// someone else locks this page, it will get removed from PC by him.
		return 0;
	}

	spin_lock(&g_free_epc_pages_lock);
	g_free_epc_pages.push_back(aptr->prm_ptr);
	spin_unlock(&g_free_epc_pages_lock);

	return 0;
}

#endif

/////*** libc-like memcopies *****///

void *memmove_reg_aptr(void *dst0, void *src0, size_t length)
{
	Aptr<char>& src = *(Aptr<char>*)src0;
	int ctr = length;

	unsigned int* dst = (unsigned int*)dst0;
	while (ctr > 0)
	{
		unsigned char* aptr_mem = deref(&src.m_aptr, src.m_base_page_index);

#ifdef APTR_RANDOM_ACCESS
		int left = SUB_PAGE_SIZE * (1 + src.m_aptr.sub_page_index) - src.m_aptr.offset;
#else
		int left = SUB_PAGE_SIZE - src.m_aptr.offset;
#endif

		if (ctr > left)
		{
			memmove((char*)dst0 + (length - ctr), aptr_mem, left);
			MOVE_APTR(&src.m_aptr, left);
			ctr -= left;
			//left = PAGE_SIZE;
		}
		else
		{
			memmove((char*)dst0 + (length - ctr), aptr_mem, ctr);
			ctr -= ctr;
		}
	}

	return dst0;
}

void *memmove_aptr_reg(void *dst0, void *src0, size_t length)
{
	Aptr<char> dst = *(Aptr<char>*)dst0;
	dst.m_aptr.hint_write = true; // this is dirty state
	int ctr = length;
	while (ctr > 0)
	{
		unsigned char* aptr_mem = deref(&dst.m_aptr, dst.m_base_page_index);

#ifdef APTR_RANDOM_ACCESS
		int left = SUB_PAGE_SIZE * (1 + dst.m_aptr.sub_page_index) - dst.m_aptr.offset;
#else
		int left = SUB_PAGE_SIZE - dst.m_aptr.offset;
#endif

		if (ctr > left)
		{
			memmove(aptr_mem, (char*)src0 + (length - ctr), left);
			MOVE_APTR(&dst.m_aptr, left);
			ctr -= left;
			//			left = PAGE_SIZE;
		}
		else
		{
			memmove(aptr_mem, (char*)src0 + (length - ctr), ctr);
			//MOVE_APTR(&dst.m_aptr, ctr);
			ctr -= ctr;
		}
	}

	return dst0;
}

int memcmp_aptr_aptr(const void* s1, const void* s2, size_t n)
{
	unsigned char u1, u2;
	Aptr<char> aptr_s1 = Aptr<char>((void*)s1, n, 0);
	Aptr<char> aptr_s2 = Aptr<char>((void*)s2, n, 0);

	for (; n--; MOVE_APTR(&aptr_s1.m_aptr, 1), MOVE_APTR(&aptr_s2.m_aptr, 1)) {
		u1 = (unsigned char)*deref(&aptr_s1.m_aptr, aptr_s1.m_base_page_index);
		u2 = (unsigned char)*deref(&aptr_s2.m_aptr, aptr_s2.m_base_page_index);
		if (u1 != u2) {
			return (u1 - u2);
		}
	}

	return 0;
}

int memcmp_reg_aptr(const void* s1, const void* s2, size_t n)
{
	unsigned char u1, u2;
	char* aptr_s1 = (char*)s1;
	Aptr<char> aptr_s2 = Aptr<char>((void*)s2, n, 0);

	for (; n--; aptr_s1++, MOVE_APTR(&aptr_s2.m_aptr, 1)) {
		u1 = (unsigned char)*aptr_s1;
		u2 = (unsigned char)*deref(&aptr_s2.m_aptr, aptr_s2.m_base_page_index);
		if (u1 != u2) {
			return (u1 - u2);
		}
	}

	return 0;
}


void *memset_aptr(void *dst, int c, size_t n)
{
	if (n != 0) {
		Aptr<char> aptr = Aptr<char>(dst, n, 0);
		aptr.m_aptr.hint_write = true;
		do
		{

			*deref(&aptr.m_aptr, aptr.m_base_page_index) = (unsigned char)c;
			MOVE_APTR(&aptr.m_aptr, 1);
		} while (--n != 0);
	}

	return (dst);
}

int strncmp_aptr(char* s1, char *s2, size_t n)
{
	Aptr<char> aptr = Aptr<char>(s1, n, 0);
	for (; n > 0; MOVE_APTR(&aptr.m_aptr, 1), s2++, --n)
	{
		unsigned char* val = deref(&aptr.m_aptr, aptr.m_base_page_index);
		if (*val != *s2)
		{
			return ((*val < *(unsigned char *)s2) ? -1 : +1);
		}
		else if (*val == '\0')
		{
			return 0;
		}
	}

	return 0;
}

void * memcpy_reg_aptr(char *dst0, const char *src0, size_t length)
{
	return memmove_reg_aptr(dst0, (void*)src0, length);
}

void * memcpy_aptr_reg(char *dst0, const char *src0, size_t length)
{
	return memmove_aptr_reg(dst0, (void*)src0, length);
}

void * memcpy_aptr(char *dst0, const char *src0, size_t length)
{
	if (length == 0 || dst0 == src0)		/* nothing to do */
		return (dst0);

	Aptr<char> dst = Aptr<char>(dst0, length, 0);
	Aptr<char> src = Aptr<char>((void*)src0, length, 0);
	dst.m_aptr.hint_write = true;

	do
	{
		*deref(&dst.m_aptr, dst.m_base_page_index) = *deref(&src.m_aptr, src.m_base_page_index);
		MOVE_APTR(&dst.m_aptr, 1);
		MOVE_APTR(&src.m_aptr, 1);
	} while (--length != 0);
	return dst0;
}

