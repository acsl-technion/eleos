/*
 * SimpleHashMap.h
 *
 *  Created on: Jul 17, 2016
 *      Author: user
 */

#ifndef _CRYPTOCACHE_H_
#define _CRYPTOCACHE_H_

#include "SyncUtils.h"
#include "DataStructures.h"
#include "mem.h"
#include <stdlib.h>

#define USED_BUCKET_SIZE 100

#pragma pack(push)
#pragma pack(1)
struct crypto_item_t
{
	int page_index;
	uint8_t nonce[NONCE_BYTE_SIZE * SUB_PAGE_COUNT];
	//sgx_aes_gcm_128bit_tag_t* mac_ptr;
	sgx_aes_gcm_128bit_tag_t mac_ptr[SUB_PAGE_COUNT];
#ifdef APTR_RANDOM_ACCESS
	bool is_initialized[SUB_PAGE_COUNT];
#endif
	crypto_item_t *next;
};
#pragma pack(pop)

struct crypto_bucket_t
{
	crypto_item_t* head;
	volatile unsigned char lock;
};

class CryptoCache
{
private:

	//static CryptoCache *s_instance;

	crypto_bucket_t* m_buckets;
	size_t m_hashmap_size;

public:
	CryptoCache()
	{
	}

	/*
	// Singleton pattern, Beware, not thread-safe!
	static CryptoCache *instance()
	{
			if (!s_instance)
			  s_instance = new CryptoCache();
			return s_instance;
	}
*/
	void init(size_t num_of_buckets)
	{
		m_hashmap_size = num_of_buckets;
		m_buckets = new crypto_bucket_t[num_of_buckets];
		for (size_t i = 0; i < num_of_buckets; i++)
		{
			m_buckets[i].head = NULL;
			m_buckets[i].lock = 0;
		}
	}

	void cleanup()
	{
		for (size_t i = 0; i < m_hashmap_size; i++)
		{
			crypto_item_t* p = m_buckets[i].head;
			while (p)
			{
				crypto_item_t* to_delete = p;
				p = p->next;
				delete to_delete;
			}
		}

		delete[] m_buckets;
		m_buckets = NULL;
	}

	crypto_item_t* get(const int& page_index)
	{
		crypto_bucket_t& bkt = m_buckets[page_index % m_hashmap_size];
		spin_lock(&bkt.lock);
		crypto_item_t *p = bkt.head;

		while (p)
		{
			if (p->page_index == page_index)
			{
				spin_unlock(&bkt.lock);
				return p;
			}

			p = p->next;
		}

		spin_unlock(&bkt.lock);
		return NULL;
	}

	crypto_item_t* add(const int& page_index)
	{
		crypto_bucket_t& bkt = m_buckets[page_index % m_hashmap_size];
		spin_lock(&bkt.lock);
		crypto_item_t *p = bkt.head;
		crypto_item_t* ret = NULL;
		crypto_item_t* tail = NULL;

		if (p == NULL) // no items yet in this bucket
		{
			crypto_item_t* it = new crypto_item_t();
			it->page_index = page_index;
			//it->mac_ptr = (sgx_aes_gcm_128bit_tag_t*) memsys5Malloc(sizeof(sgx_aes_gcm_128bit_tag_t)*SUB_PAGE_COUNT);
			it->next = NULL;

#ifdef APTR_RANDOM_ACCESS
			for (int i = 0; i < SUB_PAGE_COUNT; i++)
			{
				it->is_initialized[i] = false;
			}
#endif

			bkt.head = it;
			ret = it;
		}
		else
		{
			while (p)
			{
				if (p->page_index == page_index) // already in list, just return it.
				{
					spin_unlock(&bkt.lock);
					return p;
				}

				tail = p;
				p = p->next;
			}

			crypto_item_t* it = new crypto_item_t();
			it->page_index = page_index;
			//it->mac_ptr = (sgx_aes_gcm_128bit_tag_t*) memsys5Malloc(sizeof(sgx_aes_gcm_128bit_tag_t)*SUB_PAGE_COUNT);
			it->next = NULL;

#ifdef APTR_RANDOM_ACCESS
			for (int i = 0; i < SUB_PAGE_COUNT; i++)
			{
				it->is_initialized[i] = false;
			}
#endif

			tail->next = it;
			ret = it;
		}

		spin_unlock(&bkt.lock);

		return ret;
	}
};

#endif /* ENCLAVE_SAMPLES_TRUSTED_HASHMAP_SIMPLEHASHMAP_H_ */
