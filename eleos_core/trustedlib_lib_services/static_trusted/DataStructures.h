#ifndef _DATA_STRUCTURES_HPP_INCLUDED_
#define	_DATA_STRUCTURES_HPP_INCLUDED_

#include <sgx_tcrypto.h>

//#define APTR_RANDOM_ACCESS
#undef APTR_RANDOM_ACCESS


// First version - can use custom sizes for each item in future...
#define PAGE_SIZE 4096
#define EPC_SIZE 134217728 // 128 MB
#define CACHE_SIZE (1 << 26) // 64 MB
#define LLC_CAPACITY 262144 //65536
#define CACHE_CAPACITY (CACHE_SIZE / PAGE_SIZE)
#define NONCE_BYTE_SIZE 12

#ifndef APTR_RANDOM_ACCESS
#warning "**********************RANDOM_IS_DISABLED ************************"
#define SUB_PAGE_COUNT_LOG 0
#else
#warning "**********************RANDOM_IS_ENABLED ************************"
#define SUB_PAGE_COUNT_LOG 4
#endif

#define SUB_PAGE_COUNT (1 << SUB_PAGE_COUNT_LOG)
#define SUB_PAGE_SIZE_LOG (PAGE_SIZE_LOG - SUB_PAGE_COUNT_LOG)
#define SUB_PAGE_SIZE  (1 << SUB_PAGE_SIZE_LOG)

#endif


