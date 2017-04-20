#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */

#include "samples.h"
#include "samples_t.h"  /* print_string */
#include "microbenchmarks/memset.h"

/* 
 * printf: 
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
void printf(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_samples_sample(buf);
}

int ecall_samples_sample()
{
  printf("IN SAMPLES\n");
  return 0;
}

int ecall_preprocess_reg(int item_size, int arr_size)
{
	return preprocess_memset_regular(arr_size, item_size);
}
int ecall_preprocess_aptr(int item_size, int arr_size)
{
	return preprocess_memset_aptr(arr_size, item_size);
}

int ecall_process_reg(int item_size, int arr_size, int test_read, int test_write, int avg_times, long* timestamp, int* test_stop)
{
	return memset_regular(arr_size, item_size, test_read, test_write, avg_times, timestamp, test_stop);
}
int ecall_process_aptr(int item_size, int arr_size, int test_read, int test_write, int avg_times, long* timestamp, int* test_stop)
{
	return memset_aptr(arr_size, item_size, test_read, test_write, avg_times, timestamp, test_stop);
}
