/*
 * memset.hpp
 *
 *  Created on: Jun 23, 2016
 *      Author: user
 */

#ifndef ENCLAVE_SAMPLES_TRUSTED_MICROBENCHMARKS_MEMSET_H_
#define ENCLAVE_SAMPLES_TRUSTED_MICROBENCHMARKS_MEMSET_H_

int preprocess_native(int arr_size, int item_size);
int process_native(int arr_size, int item_size, int test_read, int test_write, int avg_times);

#endif /* ENCLAVE_SAMPLES_TRUSTED_MICROBENCHMARKS_MEMSET_H_ */
