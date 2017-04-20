/*
 * Aptr.h
 *
 *  Created on: Jul 15, 2016
 *      Author: user
 */

#ifndef TRUSTEDLIB_LIB_SERVICES_STATIC_TRUSTED_APTR_H_
#define TRUSTEDLIB_LIB_SERVICES_STATIC_TRUSTED_APTR_H_

#include "PerfCounters.h"
#include "consts.h"
#include <stdio.h>
#include "mem.h"
#include "common_utils.h"
#include "DataStructures.h"
#include <cmath>
#include <stdlib.h>
#include <sgx_trts.h>
#include <assert.h>
#include "SyncUtils.h"
#include "trusted_utils.h"
#include <sgx_tcrypto.h>
//#include <stlport/unordered_map>
//#include <stlport/list>
#include <stlport\vector>
 //#include "lib_services_t.h"
#include  <math.h>

#include "PageCache.h"
#include "CryptoCache.h"

//void debug(const char *fmt, ...);

#define MN_REQ 16
#define PAGE_SIZE_LOG 12
#define LOAD_FACTOR 32
//#define PAGE_SIZE 1 << PAGE_SIZE_LOG
#define PAGE_BITS_MASK (PAGE_SIZE-1)

#define VALID_BITS 1
#define ACCESS_BITS 1
#define OFFSET_PTR_BITS 64
#define PAGE_BITS (OFFSET_PTR_BITS - PAGE_SIZE_LOG - VALID_BITS - ACCESS_BITS)
#define VALID_MASK 0xFFFFF000

#define ISVALID(X) (((X) < PAGE_SIZE) && ((X)>=0))
//#define ISVALID(X) ((X)&VALID_MASK == 0)
#define MOVE_APTR(aptr,val) (aptr)->offset+=val
#define APTR2ADDR(APTR) (unsigned char*)(((int64_t)*(APTR).prm_ptr & 0xFFFFFFFFFFFFFFFE)+(APTR).offset)

// This class is just a wrapper, as such it is not Thread-Safe. Use Locks in order to change internal structures when working on the same Aptr concurrently
struct /*__attribute__ ((aligned (8))) */aptr_t
{
	unsigned char** prm_ptr;
	int page;
	int offset;
#ifdef APTR_RANDOM_ACCESS
	char sub_page_index;
#endif
	bool hint_write;
};

// Should be initialized to zeroes
extern PageCache g_aptr_crypto_page_cache_l1;
extern std::vector<unsigned char**> g_free_epc_pages;

extern sgx_aes_gcm_128bit_key_t g_eviction_key;
extern unsigned char* g_base_pool_ptr;
extern size_t g_pool_size;
extern volatile unsigned long long* g_perf_counters;

int initialize_aptr(void* ptr_pool, size_t _pool_size);
int cleanup_resources();

void remove_entries_pcache(int num_entries);
void page_fault(aptr_t* aptr, int base_page_index);
__forceinline unsigned char* deref(aptr_t* aptr, int base_page_index);

#ifdef APTR_RANDOM_ACCESS
int aptr_fsync(aptr_t* aptr, int base_page_index);
unsigned char* deref_direct(aptr_t* aptr, int base_page_index);
#endif

extern unsigned char DUMMY;

// This class is just a wrapper, as such it is not Thread-Safe. Use Locks in order to change internal structures when working on the same Aptr concurrently
template<typename T>
class Aptr {
public:
	void init() {
		m_size = 0, m_base_page_index = -1;

	}
	Aptr()
	{
		init();	// null aptr
	}

	Aptr(void* ram_ptr, size_t size, int notUsed) : m_size(size)
	{
		if (ram_ptr == NULL)
		{
			debug("wrong parameters in APTR init");
		}

		m_aptr.offset = ((((__int64_t)ram_ptr - (__int64_t)g_base_pool_ptr)) & PAGE_BITS_MASK) + PAGE_SIZE;
		m_aptr.page = -1;
		m_aptr.prm_ptr = (unsigned char**)&DUMMY; // DUMMY VALUE
		m_aptr.hint_write = false;
#ifdef APTR_RANDOM_ACCESS
		m_aptr.sub_page_index = 0;
#endif
		m_base_page_index = ((((__int64_t)ram_ptr - (__int64_t)g_base_pool_ptr)) >> PAGE_SIZE_LOG);

		ASSERT(m_base_page_index >= 0);
	}

	Aptr(const Aptr& aptr)
	{
		do_copy(aptr);
	}

	~Aptr()
	{
		reset();
	}


	__forceinline Aptr& operator=(const Aptr<T>& aptr)
	{
		reset();
		do_copy(aptr);

		return *this;
	}

	__forceinline Aptr& move(size_t offset)
	{
		offset *= sizeof(T);
		MOVE_APTR(&m_aptr, offset);
		return *this;
	}

	__forceinline T& operator *()
	{
		void* pRet = deref(&m_aptr, m_base_page_index);
		return *((T*)pRet);
	}

	__forceinline bool is_not_null()
	{
		return m_base_page_index > 0;
	}

	__forceinline bool operator!()
	{
		return m_base_page_index < 0;
	}

	__forceinline bool operator==(const Aptr& lhs)
	{
		return lhs.m_base_page_index == m_base_page_index;//TODO && lhs.m_aptr.offset + (lhs.m_aptr.page >> PAGE_SIZE_LOG) == m_aptr.offset + m_aptr.page >> PAGE_SIZE_LOG;
	}

	__forceinline bool operator!=(const Aptr& lhs)
	{
		return lhs.m_base_page_index != m_base_page_index;//TODO || lhs.m_aptr.offset != m_aptr.offset || lhs.m_aptr.page != m_aptr.page;
	}

	__forceinline Aptr& operator ++(int)
	{
		return move(1);
	}

	__forceinline Aptr& operator --(int)
	{
		return move(-1);
	}

	__forceinline Aptr& operator ++()
	{
		return move(1);
	}

	__forceinline Aptr& operator --()
	{
		return move(-1);
	}

	__forceinline Aptr& operator +=(size_t offset)
	{
		return move(offset);
	}

	__forceinline Aptr& operator -=(size_t offset)
	{
		return move(-offset);
	}

	__forceinline void reset()
	{
		if (m_base_page_index < 0 || *m_aptr.prm_ptr == NULL)
		{
			m_base_page_index = -1; // for double dtor's case
			m_aptr.prm_ptr = (unsigned char**)&DUMMY;

			return;
		}

#ifdef APTR_RANDOM_ACCESS
		aptr_fsync(&m_aptr, m_base_page_index);
#endif

		if (*m_aptr.prm_ptr != NULL)
		{
			int page_index = m_base_page_index + (m_aptr.page);
			g_aptr_crypto_page_cache_l1.dec_ref_count(page_index);
			//if (m_aptr.hint_write && g_page_ref_count_arr[page_index].ref_count == 0)
		/*	if (m_aptr.hint_write)
			{
				long long prm_ptr_long = (long long)*m_aptr.prm_ptr;
				if ((prm_ptr_long & 0x01) == 0)
				{
					// This will set as dirty
					prm_ptr_long |= 0x01;
					*m_aptr.prm_ptr = (unsigned char*)prm_ptr_long;

				}
			}*/
		}

		m_base_page_index = -1; // for double dtor's case
		m_aptr.prm_ptr = (unsigned char**)&DUMMY;
	}

	__forceinline void do_copy(const Aptr& aptr)
	{
		m_base_page_index = aptr.m_base_page_index;
		m_aptr.offset = aptr.m_aptr.offset + PAGE_SIZE;
		m_aptr.page = aptr.m_aptr.page - 1;
		m_aptr.prm_ptr = (unsigned char**)&DUMMY; // DUMMY VALUE- not valid by def
#ifdef APTR_RANDOM_ACCESS
		m_aptr.sub_page_index = 0;
#endif
		m_aptr.hint_write = aptr.m_aptr.hint_write;
		m_size = aptr.m_size;
	}

	//private:
	aptr_t m_aptr;
	int m_base_page_index;
	size_t m_size;
};

extern unsigned long g_dirty;

__forceinline unsigned char* deref(aptr_t* aptr, int base_page_index)
{
#ifndef APTR_RANDOM_ACCESS
	int b = ISVALID(aptr->offset);
#else
	int b = aptr->offset >= aptr->sub_page_index * SUB_PAGE_SIZE &&  aptr->offset < (aptr->sub_page_index + 1) * SUB_PAGE_SIZE;
#endif
	unsigned char* pRet;

	//if (likely(b)){
	if (b) {
		pRet = APTR2ADDR(*aptr);
	}
	else {
#ifdef APTR_RANDOM_ACCESS
		aptr_fsync(aptr, base_page_index);
#endif
		page_fault(aptr, base_page_index);
		pRet = APTR2ADDR(*aptr);
	}
#ifndef APTR_RANDOM_ACCESS
	if (aptr->hint_write)
	{
		ASSERT(*aptr->prm_ptr != NULL);
		long long prm_ptr_long = (long long)*(aptr->prm_ptr);
		prm_ptr_long |= 0x01;
		g_dirty++;
		//		debug("%lu\n",g_dirty);
		*(aptr->prm_ptr) = (unsigned char*)prm_ptr_long;
	}
#endif
	return pRet;
}

int strncmp_aptr(char* s1, char *s2, size_t n);
void * memcpy_aptr(char *dst0, const char *src0, size_t length);
void * memcpy_aptr_reg(char *dst0, const char *src0, size_t length);
void * memcpy_reg_aptr(char *dst0, const char *src0, size_t length);

void *memset_aptr(void *dst, int c, size_t n);

int memcmp_aptr_aptr(const void* s1, const void* s2, size_t n);
int memcmp_reg_aptr(const void* s1, const void* s2, size_t n);

void *memmove_aptr_reg(void *dst0, void *src0, size_t length);
void *memmove_reg_aptr(void *dst0, void *src0, size_t length);

#endif /* TRUSTEDLIB_LIB_SERVICES_STATIC_TRUSTED_APTR_H_ */
