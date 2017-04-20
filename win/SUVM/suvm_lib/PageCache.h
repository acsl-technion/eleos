/*
 * SimpleHashMap.h
 *
 *  Created on: Jul 17, 2016
 *      Author: user
 */

#ifndef _PAGECACHE_H_
#define _PAGECACHE_H_

#include "SyncUtils.h"
#include "common_utils.h"
#include "DataStructures.h"

#pragma pack(push)
#pragma pack(1)
struct item_t
{
	int page_index;
	unsigned char** epc;
#ifdef APTR_RANDOM_ACCESS
	bool is_valid[SUB_PAGE_COUNT];
#endif
	item_t *next;
};
#pragma pack(pop)

struct bucket
{
	item_t* head;
	volatile unsigned char lock;
};

class UsedPagesQueue
{
private:
	int m_queue_size;
	unsigned long front, rear;
	int* q;
	unsigned int volatile _lock;
public:
	UsedPagesQueue(int queue_size)
	{
		m_queue_size = queue_size;
		q = new int[queue_size];
		front = rear = 0;

		for (int i = 0; i < queue_size; i++)
		{
			q[i] = -1;
		}
	}

	~UsedPagesQueue()
	{
		delete[] q;
	}

	int enqueue(const int& elem)
	{
		spin_lock(&_lock);

		if (rear - front == m_queue_size)
		{
			spin_unlock(&_lock);
			ASSERT(false);
			return -1;
		}

		q[rear % m_queue_size] = elem;
		++rear;

		spin_unlock(&_lock);
		return 0;

	}

	int dequeue()
	{
		spin_lock(&_lock);

		if (front == rear)
		{
			spin_unlock(&_lock);
			ASSERT(false);
			return -1;
		}

		int result = q[front % m_queue_size];
		++front;

		spin_unlock(&_lock);

		return result;
	}

};

class PageCache
{
private:

	//static PageCache *s_instance;

	bucket* m_buckets;
	size_t m_hashmap_size;

	//size_t g_backing_store_size;

		// Bitmap of reference counting for all pages in the backing store. Should probably be static
	UsedPagesQueue* m_used_pages;
	volatile unsigned char* volatile m_ref_count;

public:


	PageCache()
	{
	}

	// Assuming page cache size smaller than 2^sizeof(int).
	int m_page_cache_size;
	volatile unsigned int m_page_cache_size_lock;

	// Singleton pattern, Beware, not thread-safe!
	/*
	static PageCache *instance()
	{
			if (!s_instance)
			  s_instance = new PageCache();
			return s_instance;
	}
	*/
	void init(size_t num_of_buckets, size_t backing_store_size, size_t page_cache_size)
	{
		m_page_cache_size = 0;
		m_page_cache_size_lock = 0;
		m_hashmap_size = num_of_buckets;
		m_buckets = new bucket[num_of_buckets];
		for (size_t i = 0; i < num_of_buckets; i++)
		{
			m_buckets[i].head = NULL;
			m_buckets[i].lock = 0;
		}

		m_ref_count = new volatile unsigned char[backing_store_size];
		memset((char*)m_ref_count, 0, backing_store_size);
		m_used_pages = new UsedPagesQueue(page_cache_size);
	}

	void cleanup()
	{
		delete[] m_ref_count;
		delete m_used_pages;

		for (size_t i = 0; i < m_hashmap_size; i++)
		{
			item_t* p = m_buckets[i].head;
			while (p)
			{
				item_t* to_delete = p;
				p = p->next;
				delete to_delete;
			}
		}

		delete[] m_buckets;
		m_buckets = NULL;
	}

	// TODO: inline
	void inc_ref_count(const int& page_index)
	{
		bucket& bkt = m_buckets[page_index % m_hashmap_size];
		spin_lock(&bkt.lock);
		m_ref_count[page_index]++;
		spin_unlock(&bkt.lock);
	}

	// TODO: inline
	void dec_ref_count(const int& page_index)
	{
		bucket& bkt = m_buckets[page_index % m_hashmap_size];
		spin_lock(&bkt.lock);
		m_ref_count[page_index]--;
		spin_unlock(&bkt.lock);
	}

	// This method is reponsible for the eviction policy and removing the page from the page cache
	// Hack: This will lock the bucket, the unlock happens in remove
	item_t* get_page_index_to_evict()
	{
		// Only evicting pages with ref_count == 0.
		int page_index = -1;
		do
		{
			page_index = m_used_pages->dequeue();

			bucket& bkt = m_buckets[page_index % m_hashmap_size];
			spin_lock(&bkt.lock);

			if (m_ref_count[page_index] > 0)
			{
				spin_unlock(&bkt.lock);
				m_used_pages->enqueue(page_index);
			}
			else
			{
				item_t* ret = get_internal(page_index);
				ASSERT(ret != NULL);
				return ret;
			}
		} while (true);
	}

	item_t* get_internal(const int& page_index)
	{
		bucket& bkt = m_buckets[page_index % m_hashmap_size];
		item_t *p = bkt.head;

		while (p)
		{
			if (p->page_index == page_index)
			{
				return p;
			}

			p = p->next;
		}

		return NULL;
	}

	item_t* get(const int& page_index)
	{
		bucket& bkt = m_buckets[page_index % m_hashmap_size];
		spin_lock(&bkt.lock);
		item_t* ret = get_internal(page_index);
		spin_unlock(&bkt.lock);
		return ret;
	}
#ifdef APTR_RANDOM_ACCESS
	bool try_add(const int& page_index, unsigned char**& epc_page, int sub_page_index)
#else
	bool try_add(const int& page_index, unsigned char**& epc_page)
#endif
	{
		bucket& bkt = m_buckets[page_index % m_hashmap_size];
		spin_lock(&bkt.lock);
		item_t *p = bkt.head;
		item_t* tail = NULL;

		if (p == NULL) // no items yet in this bucket
		{
			item_t* it = new item_t();
			it->page_index = page_index;
			it->epc = epc_page;
#ifdef APTR_RANDOM_ACCESS
			for (int i = 0; i < SUB_PAGE_COUNT; i++)
			{
				it->is_valid[i] = false;
			}
			it->is_valid[sub_page_index] = true;
#endif
			it->next = NULL;
			bkt.head = it;
		}
		else
		{
			while (p)
			{
				if (p->page_index == page_index) // already in list, just return it.
				{
					spin_unlock(&bkt.lock);
					return false;
				}

				tail = p;
				p = p->next;
			}

			item_t* it = new item_t();
			it->page_index = page_index;
			it->epc = epc_page;
			it->next = NULL;

#ifdef APTR_RANDOM_ACCESS
			for (int i = 0; i < SUB_PAGE_COUNT; i++)
			{
				it->is_valid[i] = false;
			}
			it->is_valid[sub_page_index] = true;
#endif

			tail->next = it;
		}

		spin_lock(&m_page_cache_size_lock);
		m_page_cache_size++;
		spin_unlock(&m_page_cache_size_lock);
#ifndef APTR_RANDOM_ACCESS
		m_used_pages->enqueue(page_index);
#endif
		spin_unlock(&bkt.lock);

		return true;
	}

	bool try_remove(const int& page_index)
	{
		bucket& bkt = m_buckets[page_index % m_hashmap_size];
		spin_lock(&bkt.lock);

		if (m_ref_count[page_index] == 1)
		{
			remove(page_index);
			return true;
		}

		spin_unlock(&bkt.lock);
		return false;
	}

	int remove(const int& page_index)
	{
		bucket& bkt = m_buckets[page_index % m_hashmap_size];
		item_t* p = bkt.head;
		item_t* prev = NULL;

		while (p)
		{
			if (p->page_index == page_index)
			{
				spin_lock(&m_page_cache_size_lock);
				m_page_cache_size--;
				spin_unlock(&m_page_cache_size_lock);

				if (prev == NULL) // head of the list
				{
					if (p->next == NULL) // single item in this bucket
					{
						bkt.head = NULL;
						delete p;
					}
					else // replace head of the bucket
					{
						bkt.head = p->next;
						delete p;
					}
				}
				else // replace linkage
				{
					prev->next = p->next;
					delete p;
				}

				spin_unlock(&bkt.lock);
				return 0;
			}

			prev = p;
			p = p->next;
		}

		ASSERT(false);
	}

};

#endif /* ENCLAVE_SAMPLES_TRUSTED_HASHMAP_SIMPLEHASHMAP_H_ */
