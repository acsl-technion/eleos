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

void spin_lock(int volatile *p)
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

#define HASHMAPSIZE ((512*1024*1024) / sizeof(item_t))

//#define USE_LINKEDLIST

#ifdef USE_LINKEDLIST

struct item_t
{
	long long key;
	long long val;
	item_t *next;
}; // TODO: attribute
//
//#define HASHMAPSIZE ((1024*1024) / sizeof(item_t))

class HashMap
{
private:
	item_t **buckets;

	// TODO: use crc32 or something else...
	inline size_t hash_key(const long long& key)
	{
		size_t t = key % HASHMAPSIZE;
		return t;
	}

public:
	HashMap()
	{
		buckets = new item_t*[HASHMAPSIZE];
	}

	~HashMap()
	{
		delete [] buckets;
	}


	item_t* get(const long long& key)
	{
		item_t *p = buckets[hash_key(key)];
		while (p)
		{
			if (p->key == key)
			{
				return p;
			}
			else
			{
				p = p->next;
			}
		}
		return NULL;
	}


	void add(const item_t &it)
	{
		item_t *newpair = new item_t();
		newpair->key = it.key;
		newpair->val = it.val;

		size_t keyhash = hash_key(it.key);

		item_t *p = buckets[keyhash];
		if (!p)
		{
			buckets[keyhash] = newpair;
		}
		else
		{
			while (p->next) p = p->next;
			p->next = newpair;
		}
	}

};

#else

struct item_t
{
	long long key;
	long long val;
}; // TODO: attribute

class HashMap
{
public:
	item_t *buckets;

	// TODO: use crc32 or something else...
	inline size_t hash_key(const long long& key)
	{
		size_t t = key % HASHMAPSIZE;
		return t;
	}

//public:
	HashMap()
	{
		buckets = new item_t[HASHMAPSIZE];
		for (size_t i=0;i<HASHMAPSIZE;i++)
		{
			buckets[i].key =-1;
		}
	}

	~HashMap()
	{
		delete [] buckets;
	}

	inline item_t* get(const long long& key)
	{
		size_t loc = hash_key(key);
		//item_t* pRet = buckets;
		//return pRet +loc;

		item_t* i = &buckets[loc];
		while (true)
		{
			if (i->key == key)
			{
				return i;
			}
			else
			{
				loc = (loc+1) % HASHMAPSIZE;
			}
		}
		return NULL;
	}

	void add(const item_t& new_item)
	{
		int i=0;
		while (true)
		{
			size_t keyhash = hash_key(new_item.key+i);
			item_t old_item = buckets[keyhash];

			if (old_item.key == new_item.key || old_item.key == -1)
			{
				buckets[keyhash].key = new_item.key;
				buckets[keyhash].val = new_item.val;
				return;
			}
			else
			{
				i++;
			}
		}
	}
};

#endif

#endif /* ENCLAVE_SAMPLES_TRUSTED_HASHMAP_SIMPLEHASHMAP_H_ */
