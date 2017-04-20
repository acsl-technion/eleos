//#define TEST_SINGLE_APTR

//#define NOSANITY
//#define ALIGNED_ACCESS

#include "memset.h"
#include <Aptr.h>
#include <stdlib.h>
#include "../samples.h"
#include "mem.h"
#include <sgx_trts.h>

#ifdef VERIFY_ALIGNEMENT
extern unsigned char* base_pool_ptr;
#endif


#define ASSERT(cond)\
	do\
{\
	if (!(cond))\
	{\
		debug("ASSERTION FAILED: cond = %s, file = %s, line = %d, 0 = %d\n", #cond, __FILE__, __LINE__, 0);\
		abort();\
	}\
} while(0)  

//#define VERIFY_APTR
int verify_correct_write(char expected_val, int item_size, int arr_size);


volatile int g_acc =0;

#ifdef TEST_SINGLE_APTR
int* g_reg_buffer;
int* g_aptr_buffer;

//#define STUPID_TEST_TO_SEE_DRIVER_PAGE_FAULTS

int preprocess_memset_regular(int arr_size, int item_size)
{	
	g_reg_buffer = (int*)malloc(arr_size * item_size * sizeof(int));
	ASSERT(g_reg_buffer);

#ifdef STUPID_TEST_TO_SEE_DRIVER_PAGE_FAULTS
	int times = 1000;
	abort(); // to measure this long warmup
	while(times--) {
#endif

		for(int i = 0 ; i < arr_size * item_size ; i++) {
			g_reg_buffer[i]=i;
		}

#ifdef STUPID_TEST_TO_SEE_DRIVER_PAGE_FAULTS
	}
	abort();
#endif

	return 0;
}

int memset_regular(int arr_size, int item_size, int test_read, int test_write, int avg_times, volatile long* timestamp, volatile int* test_stop)
{
	do {
		for (int x=0 ; x < avg_times ; x++)
		{
			unsigned int i;
			sgx_read_rand((unsigned char*)&i, sizeof(i));
			i %= (arr_size-1)*item_size;

			int* p = g_reg_buffer + i;

#ifdef ALIGNED_ACCESS
			p = (int*)((unsigned long)p & ~(item_size - 1));
			if(p < g_reg_buffer)
				p = g_reg_buffer;
#endif

			long checksum = 0;

			int priv[4096];
			if (test_read){
				memmove(priv,p,item_size*sizeof(int));	
#ifndef NOSANITY		

				for (int j=0;j<item_size;j++)
				{
					if(priv[j]!=i+j) checksum += 1;
				}
#else 
#warning "**********************NO SANITY CHECKS WILL BE PERFORMED ************************"
#endif
			}
			if (test_write){

#ifndef NOSANITY		
				for (int j=0;j<item_size;j++)
				{
					priv[j]=i+j; 
				}
#endif				
				memmove(p,priv,item_size*sizeof(int));	
			}
			ASSERT(checksum==0);

			__sync_fetch_and_add(timestamp, 1);

			if (*test_stop > 0) return 0;

		}

	} while(timestamp);

	return g_acc;
}

int preprocess_memset_aptr(int arr_size, int item_size)
{
	g_aptr_buffer = (int*)memsys5Malloc(item_size * arr_size * sizeof(int));
	ASSERT(g_aptr_buffer);
	Aptr<int> ap(g_aptr_buffer,item_size * arr_size * sizeof(int), 0);
	ap.m_aptr.hint_write = 1;

	debug("arr_size = %d\n", arr_size * item_size);
	for(int i = 0 ; i < arr_size * item_size ; i++) {

		unsigned int* val = (unsigned int*) deref(&ap.m_aptr, ap.m_base_page_index);
		*val = i;

		MOVE_APTR(&ap.m_aptr, sizeof(int));
	}

	debug("done preprocess_memset_aptr()\n");

	return 0;
}

int memset_aptr(int arr_size, int item_size, int test_read, int test_write, int avg_times, volatile long* timestamp, volatile int* test_stop)
{
#ifdef ALIGNED_ACCESS
	g_aptr_buffer = (int*)(((unsigned long)g_aptr_buffer + 0x1000l) & ~0xFFF);
	// +0x1000 is because we have spare space after the allocated buffer, because memsys5Malloc doesn't use it
#endif
	do {

		for (int x=0 ; x < avg_times ; x++)
		{
			Aptr<int> ap(g_aptr_buffer,item_size * arr_size * sizeof(int) , 0);

			unsigned int i;
			sgx_read_rand((unsigned char*)&i, sizeof(i));
			i %= (arr_size-1)*item_size;

#ifdef ALIGNED_ACCESS
			i &= ~(item_size - 1);
#endif

			MOVE_APTR(&ap.m_aptr, i * sizeof(int));
			long checksum = 0;
			int priv[4096];

			if (test_write){
#ifndef NOSANITY
				for (int j=0;j<item_size;j++){
					priv[j]=i+j;
				}
#endif
				ap.m_aptr.hint_write = 1;
				memcpy_aptr_reg((char*)&ap,(char*)priv,sizeof(int)*item_size);
			}

			if (test_read){
				memcpy_reg_aptr	((char*)priv,(char*)&ap,sizeof(int)*item_size);
#ifndef NOSANITY
				for (int j=0;j<item_size;j++){
					if (priv[j]!=i+j){ debug("Sanity check failed!"); 

						debug("*val = %d\ti = %d\tj = %d\n",  priv[j], i, j);
						debug("ap.m_base_page_index = %p\n", ap.m_base_page_index);
						debug("ap.m_aptr.offset = %p\n", ap.m_aptr.offset);
						debug("*(ap.m_aptr.prm_ptr) = %p\n", *(ap.m_aptr.prm_ptr));


						ASSERT(0);
					}
				}
#endif

			}

			__sync_fetch_and_add(timestamp, 1 );
			if (*test_stop > 0) return 0;
		}

	} while(timestamp);

	return 0;
}

#else // TEST_SINGLE_APTR

Aptr<int>* g_aptr_arr;
int **g_reg_arr;

int preprocess_memset_regular(int arr_size, int item_size)
{	
	g_reg_arr = (int**)malloc(arr_size * sizeof(int*));
	ASSERT(g_reg_arr);

#ifdef STUPID_TEST_TO_SEE_DRIVER_PAGE_FAULTS
	int times = 1000;
	abort(); // to measure this long warmup
	while(times--) {
#endif

		for(int i = 0 ; i < arr_size  ; i++) 
		{
			int* t=g_reg_arr[i]=(int*)malloc(item_size*sizeof(int));
			ASSERT(t);

			for(int j=0;j<item_size;j++)
			{
				*(t+j)=i*item_size+j;
			}

		}

#ifdef STUPID_TEST_TO_SEE_DRIVER_PAGE_FAULTS
	}
	abort();
#endif

	return 0;
}

int memset_regular(int arr_size, int item_size, int test_read, int test_write, int avg_times, volatile long* timestamp, volatile int* test_stop)
{
	for (int x=0 ; x < avg_times ; x++)
	{
		unsigned int i;
		sgx_read_rand((unsigned char*)&i, sizeof(i));
		i %= arr_size;

		int* p = g_reg_arr[i];
		long checksum = 0;

		int priv[4096];
		if (test_read){
			memmove(priv,p,item_size*sizeof(int));	
#ifndef NOSANITY		

			for (int j=0;j<item_size;j++)
			{
				if(priv[j]!=i*item_size+j) checksum += 1;
			}
#else 
#warning "**********************NO SANITY CHECKS WILL BE PERFORMED ************************"
#endif
		}
		if (test_write){

#ifndef NOSANITY		
			for (int j=0;j<item_size;j++)
			{
				priv[j]=i*item_size+j; 
			}
#endif				
			memmove(p,priv,item_size*sizeof(int));	
		}
		ASSERT(checksum==0);

		__sync_fetch_and_add(timestamp, 1 );
	}

	return g_acc;
}

int preprocess_memset_aptr(int arr_size, int item_size)
{
	g_aptr_arr = (Aptr<int>*)memsys5Malloc( arr_size*sizeof(Aptr<int>));
	ASSERT(g_aptr_arr);

	int** mem_array = new int*[arr_size];
	debug("arr_size = %d\n", arr_size );
	for(int i = 0 ; i < arr_size ; i++) {
		int* tmpmem=(int*)memsys5Malloc(item_size*sizeof(int));
		ASSERT(tmpmem);
		mem_array[i]=tmpmem;
	}	
	for (int i=0;i<arr_size;i++) {
		Aptr<int> ap(mem_array[i],item_size *sizeof( int), 0);
		ap.m_aptr.hint_write = 1;

		g_aptr_arr[i].init();
		g_aptr_arr[i]=ap;
		for(int j=0;j<item_size;j++){
			unsigned int* val = (unsigned int*) deref(&ap.m_aptr, ap.m_base_page_index);
			*val = i*item_size+j;
			MOVE_APTR(&ap.m_aptr, sizeof(int));
		}
		g_aptr_arr[i].m_aptr.hint_write = 0;
	}

	delete[] mem_array;

	debug("done preprocess_memset_aptr()\n");

	return 0;
}

int memset_aptr(int arr_size, int item_size, int test_read, int test_write, int avg_times, volatile long* timestamp, volatile int* test_stop)
{

	for (int x=0 ; x < avg_times ; x++)
	{

		unsigned int i;
		sgx_read_rand((unsigned char*)&i, sizeof(i));
		i %= arr_size;
		Aptr<int> ap=g_aptr_arr[i];
		long checksum = 0;
		int priv[4096];

		if (test_write){
#ifndef NOSANITY
			for (int j=0;j<item_size;j++){
				priv[j]=i*item_size+j;
			}
#endif
			ap.m_aptr.hint_write = 1;
			memcpy_aptr_reg((char*)&ap,(char*)priv,sizeof(int)*item_size);
		}

		if (test_read){

			memcpy_reg_aptr	((char*)priv,(char*)&ap,sizeof(int)*item_size);
#ifndef NOSANITY
			for (int j=0;j<item_size;j++){
				if (priv[j]!=i*item_size+j){ debug("Sanity check failed!"); 


					debug("*val = %d\ti = %d\tj = %d\n",  priv[j], i, j);
					debug("ap.m_base_page_index = %p\n", ap.m_base_page_index);
					debug("ap.m_aptr.offset = %p\n", ap.m_aptr.offset);
					debug("*(ap.m_aptr.prm_ptr) = %p\n", *(ap.m_aptr.prm_ptr));


					ASSERT(0);
				}
			}
#endif

		}
		__sync_fetch_and_add(timestamp, 1 );
	}

	return 0;
}
#endif // TEST_SINGLE_APTR
