/*
 * mem_tester.cpp
 *
 *  Created on: Jul 15, 2016
 *      Author: user
 */

#include "samples.h"
#include "mem_tester.h"

int test_memsys5Malloc()
{
	bool res = true;
	void* ptr = memsys5Malloc(64);
	res = res & (ptr != NULL);

	memsys5Free(ptr);

	return res;
}

int test_memsys5Roundup()
{
	bool res = true;
	void* ptr = memsys5Malloc(64);
	res = res & (ptr != NULL);
	int x = memsys5Roundup(128);
	res = res & (x != 0);
	memsys5Free(ptr);

	return res;
}

int test_memsys5Size()
{
	bool res = true;
	void* ptr = memsys5Malloc(64);
	res = res & (ptr != NULL);
	int x = memsys5Size(ptr);
	res = res & (x == 64);
	memsys5Free(ptr);

	return res;
}

int test_page_alignment(void* base_ptr)
{
	bool res = true;
	void* ptr1 = memsys5Malloc(64);
	res = res & (ptr1 != NULL);
	void* ptr2 = memsys5Malloc(64);
	res = res & (ptr2 != NULL);
	void* ptr3 = memsys5Malloc(4096);
	res = res & (ptr3 != NULL);
	void* ptr4 = memsys5Malloc(256);
	res = res & (ptr3 != NULL);
	void* ptr5 = memsys5Malloc(3776); // to big for same page
	res = res & (ptr3 != NULL);

	// ptr1&ptr2 should be in the same page, ptr3 in a different page, ptr 4 should be with ptr1&2, ptr5 should be in a different page
	int p1_page_index = (((__int64_t)ptr1 - (__int64_t)base_ptr) / 4096);
	int p2_page_index = (((__int64_t)ptr2 - (__int64_t)base_ptr) / 4096);
	int p3_page_index = (((__int64_t)ptr3 - (__int64_t)base_ptr) / 4096);
	int p4_page_index = (((__int64_t)ptr4 - (__int64_t)base_ptr) / 4096);
	int p5_page_index = (((__int64_t)ptr5 - (__int64_t)base_ptr) / 4096);

	res &= p1_page_index == p2_page_index;
	res &= p3_page_index != p2_page_index;
	res &= p4_page_index == p2_page_index;
	res &= p5_page_index != p2_page_index;
	res &= p5_page_index != p3_page_index;

	memsys5Free(ptr1);
	memsys5Free(ptr2);
	memsys5Free(ptr3);
	memsys5Free(ptr4);
	memsys5Free(ptr5);

	return res;
}
