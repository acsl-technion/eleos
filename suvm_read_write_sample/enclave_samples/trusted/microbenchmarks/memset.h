/*
 * memset.hpp
 *
 *  Created on: Jun 23, 2016
 *      Author: user
 */

#ifndef ENCLAVE_SAMPLES_TRUSTED_MICROBENCHMARKS_MEMSET_H_
#define ENCLAVE_SAMPLES_TRUSTED_MICROBENCHMARKS_MEMSET_H_

int preprocess_memset_regular(int arr_size, int item_size);
int memset_regular(int arr_size, int item_size, int test_read, int test_write, int avg_times, volatile long* timestamp, volatile int* test_stop);
int preprocess_memset_aptr(int arr_size, int item_size);
int memset_aptr(int arr_size, int item_size, int test_read, int test_write, int avg_times, volatile long* timestamp, volatile int* test_stop);

#endif /* ENCLAVE_SAMPLES_TRUSTED_MICROBENCHMARKS_MEMSET_H_ */
