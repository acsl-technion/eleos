/*
 * memset.cpp
 *
 *  Created on: Jun 23, 2016
 *      Author: user
 */

#include "memset.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

long seed;

inline double nextDouble(int max)
{
    // Robert Jenkins' 32 bit integer hash function.
    seed = ((seed + 0x7ed55d16) + (seed << 12)) & 0xffffffff;
    seed = ((seed ^ 0xc761c23c) ^ (seed >> 19)) & 0xffffffff;
    seed = ((seed + 0x165667b1) + (seed << 5)) & 0xffffffff;
    seed = ((seed + 0xd3a2646c) ^ (seed << 9)) & 0xffffffff;
    seed = ((seed + 0xfd7046c5) + (seed << 3)) & 0xffffffff;
    seed = ((seed ^ 0xb55a4f09) ^ (seed >> 16)) & 0xffffffff;
    return max * static_cast <double> (seed & 0xfffffff) / static_cast <double> (0x10000000);
}

#define SEED_SIZE sizeof(long)

char** buf1;
volatile int acc =0;

int preprocess_native(int arr_size, int item_size)
{
	srand(time(NULL));

	buf1 = new char*[arr_size];
	for (int i=0;i<arr_size;i++)
	{
		buf1[i] = new char[item_size];
		memset(buf1[i], 5, item_size);
	}

	seed = rand();

	return 0;
}

int process_native(int arr_size, int item_size, int test_read, int test_write, int avg_times)
{
	char* it;
	for (int x=0;x<avg_times;x++)
	{
		long i = nextDouble(arr_size);

		it = buf1[i];
		for (int j=0;j<item_size;j++)
		{
			if (test_read)
				acc += *it;
			if (test_write)
			{
				*it = 3;
			}
			it++;
		}
	}

	return acc;
}
