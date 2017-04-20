/*
 * SimpleHashMap.h
 *
 *  Created on: Jul 17, 2016
 *      Author: user
 */

#ifndef ENCLAVE_SAMPLES_TRUSTED_HASHMAP_SIMPLEHASHMAP_H_
#define ENCLAVE_SAMPLES_TRUSTED_HASHMAP_SIMPLEHASHMAP_H_

#include "../suvm_lib_common/SyncUtils.h"
#include "DataStructures.h"

#define USED_BUCKET_SIZE 100

template<typename Tval>
struct item_t
{
        int page_index;
        Tval value;
        //void* epc_page;
        item_t *next;
}; // TODO: attribute

template<typename Tval>
struct bucket
{
        item_t<Tval>* head;
        volatile unsigned char lock;
};


template<typename Tval>
class HashMap
{
private:
	bucket<Tval>* m_buckets;
	size_t m_hashmap_size;
public:

	HashMap(size_t hashmap_size)
	{
		m_hashmap_size = hashmap_size;
		m_buckets = new bucket<Tval>[hashmap_size];
		for (size_t i=0;i<hashmap_size;i++)
                {
                        item_t<Tval>* it = new item_t<Tval>();
			it->page_index=-1;
			it->next = NULL;
			m_buckets[i].head = it;
			m_buckets[i].lock = 0;
                }
	}

	~HashMap()
	{
		for (size_t i=0;i<m_hashmap_size;i++)
		{			
			item_t<Tval>* p = m_buckets[i].head;
			while (p)
			{
				item_t<Tval>* to_delete = p;
				p = p->next;
				delete to_delete;
			}	
		}

		delete[] m_buckets;
		m_buckets = NULL;
	}


	item_t<Tval>* get(const int& page_index)
	{
		bucket<Tval> bkt = m_buckets[page_index % m_hashmap_size];
		spin_lock(&bkt.lock);
		item_t<Tval> *p = bkt.head;
	
		do {
			if (p->page_index == page_index)
			{
	                        spin_unlock(&bkt.lock);
				return p;
			}
			else
			{
				p = p->next;
			}
		} while (p);

                spin_unlock(&bkt.lock);
		return NULL;
	}

	item_t<Tval>* try_add(const int& page_index) //, void*& epc_page)
	{
		bucket<Tval> bkt = m_buckets[page_index % m_hashmap_size];
		spin_lock(&bkt.lock);
		item_t<Tval> *p = bkt.head;
		item_t<Tval>* ret = NULL;

		if (p->page_index == -1)
		{
			p->page_index = page_index;
			//p->epc_page = epc_page;
			ret = p;
		}
		else
		{
			// If page already added by other thread, try_add should fail.
			if (p->page_index == page_index)
			{				
		                spin_unlock(&bkt.lock);
				return NULL;
			}

			while (p->next) p = p->next;
			item_t<Tval>* it = new item_t<Tval>();
			it->page_index = page_index;
			//it->epc_page = epc_page;
			it->next = NULL;			

			p->next = it;
			ret = p;
		}

		spin_unlock(&bkt.lock);

		return ret;
	}

	int remove(const int& page_index)
	{
		bucket<Tval> bkt = m_buckets[page_index % m_hashmap_size];
		spin_lock(&bkt.lock);
                item_t<Tval>* p = bkt.head;
		item_t<Tval>* prev = NULL;	
		
		do {
                        if (p->page_index == page_index)
                        {
				if (prev == NULL) // head of the list
				{
					if (p->next == NULL) // single item in the list
					{
						p->page_index = -1;
						p->next = NULL;
					}
					else
					{	
						bkt.head = p->next;
						delete p;
						
					}
				}
				else
				{
					prev->next = p->next;
					delete p;
				}
                                spin_unlock(&bkt.lock);
                                return 0;
                        }
                        else
                        {
				prev = p;
                                p = p->next;
                        }
                } while (p);

		spin_unlock(&bkt.lock);
	}

};

#endif /* ENCLAVE_SAMPLES_TRUSTED_HASHMAP_SIMPLEHASHMAP_H_ */
