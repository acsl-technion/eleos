/*
 * mem_tester.h
 *
 *  Created on: Jul 15, 2016
 *      Author: user
 */

#ifndef ENCLAVE_SAMPLES_TRUSTED_MEM_TESTER_H_
#define ENCLAVE_SAMPLES_TRUSTED_MEM_TESTER_H_

#include <mem.h>

int test_memsys5Malloc();
int test_memsys5Roundup();
int test_memsys5Size();
int test_page_alignment(void* pool_ptr);

#endif /* ENCLAVE_SAMPLES_TRUSTED_MEM_TESTER_H_ */
