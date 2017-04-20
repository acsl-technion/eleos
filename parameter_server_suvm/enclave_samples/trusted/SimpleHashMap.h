/*
 * SimpleHashMap.h
 *
 *  Created on: Jul 17, 2016
 *      Author: user
 */

#ifndef ENCLAVE_SAMPLES_TRUSTED_HASHMAP_SIMPLEHASHMAP_H_
#define ENCLAVE_SAMPLES_TRUSTED_HASHMAP_SIMPLEHASHMAP_H_

#include <cstddef>
#include <stdio.h>
#include <string>
#include <functional>
#include <cassert>
#include "mem.h"
#include "Aptr.h"
#include "samples_t.h"  /* print_string */

void parameter_server_spin_lock(int volatile *p)
{
    while(!__sync_bool_compare_and_swap(p, 0, 1))
    {
        while(*p) __asm__("pause");
    }
}

#define VAL_SIZE 60
#define CACHE_LINE_SIZE 64
#define NUM_OF_ITEMS 10000  //(HASHMAPSIZE) // / CACHE_LINE_SIZE)
#define EVICT
#define AVERAGE_TIMES 100000

#define HASHMAPSIZE ((512*1024*1024) / sizeof(item_tl))

struct item_tl
{
	long long key;
	long long val;
}; // TODO: attribute

class HashMap
{
public:
	item_tl *buckets;
	Aptr<char> m_aptr;
	int aptr_start_offset;
	int aptr_start_page;

	// TODO: use crc32 or something else...
	inline size_t hash_key(const long long& key)
	{
		size_t t = key;// % HASHMAPSIZE;
		return t;
	}

//public:
	HashMap()
	{
		buckets = (item_tl*)memsys5Malloc(HASHMAPSIZE * sizeof(item_tl));
		if (buckets == NULL)
		{
			abort();
		}

		m_aptr = Aptr<char>(buckets, HASHMAPSIZE * sizeof(item_tl), 0);
		aptr_start_offset = m_aptr.m_aptr.offset;
		aptr_start_page = m_aptr.m_aptr.page;
		m_aptr.m_aptr.hint_write = true;

		for (size_t i=0;i<HASHMAPSIZE;i++)
		{
			item_tl* ptr = (item_tl*)deref(&m_aptr.m_aptr, m_aptr.m_base_page_index);
			if (m_aptr.m_aptr.offset > 4080)
			{
				printf("BUG: offset %d\n", m_aptr.m_aptr.offset);
				abort();
			}
			ptr->key = i;
			ptr->val = 0;

			MOVE_APTR(&m_aptr.m_aptr, sizeof(item_tl));
//			buckets[i].key =-1;
		}
	}

	~HashMap()
	{
		memsys5Free(buckets);
	}

//	inline Aptr<char>* get(const long long& key)
	inline void get(const long long& key)
	{
		size_t loc = hash_key(key);

		while (true)
		{
//			m_aptr.m_aptr.page = aptr_start_page;
//			m_aptr.m_aptr.offset = aptr_start_offset;
//			MOVE_APTR(&m_aptr.m_aptr, loc * sizeof(item_tl));
//			item_tl* ptr = (item_tl*) deref(&m_aptr.m_aptr, m_aptr.m_base_page_index);

			Aptr<char> tmp = Aptr<char>(buckets, HASHMAPSIZE * sizeof(item_tl), 0);
			tmp.m_aptr.hint_write = 1;
			MOVE_APTR(&tmp.m_aptr, loc * sizeof(item_tl));
			item_tl* ptr = (item_tl*) deref(&tmp.m_aptr, tmp.m_base_page_index);

//			printf("before\n");

			if (ptr->key == key)
			{
//				printf("after\n");

//				return &m_aptr;

//				printf("aptr base page: %d, key: %ld, loc: %ld\n", m_aptr.m_base_page_index, ptr->key, key);

				ptr->val++;
				return;
			}
			else
			{
				loc = (loc+1) % HASHMAPSIZE;
			}
		}

		return;
	}

	void add(const item_tl& new_item)
	{
		int i=0;
		while (true)
		{
			size_t keyhash = hash_key(new_item.key+i);
			Aptr<item_tl> old_item = Aptr<item_tl>(&buckets[keyhash], sizeof(item_tl), 0);
			printf("aptr base page: %d\n", old_item.m_base_page_index);
			item_tl* ptr = (item_tl*)deref(&old_item.m_aptr, old_item.m_base_page_index);
			printf("ptr key: %d\n", ptr->key);

			if (ptr->key == new_item.key || ptr->key == -1)
			{
				printf("got ptr: %p\n", ptr);

				old_item.m_aptr.hint_write = true;

				ptr->key = new_item.key;
				ptr->val = new_item.val;

				return;
			}
			else
			{
				i++;
			}
		}
	}
};

#endif /* ENCLAVE_SAMPLES_TRUSTED_HASHMAP_SIMPLEHASHMAP_H_ */
