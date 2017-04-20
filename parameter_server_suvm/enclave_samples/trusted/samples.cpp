#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */

#include "samples.h"
#include "samples_t.h"  /* print_string */
#include "SimpleHashMap.h"
#include <sgx_tcrypto.h>

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

HashMap* hashmap;

volatile int* start;
volatile int* finish;
int rpc;
int ocall_freq;
unsigned char* request_buffer;
unsigned char* decrypted_request_buffer;
int request_buffer_size;
unsigned char* temp;
int decrpyt_requests;

void ecall_init_hashmap(int* _start, int* _finish,int _rpc,int _ocall_freq, char* _request_buffer, int _request_buffer_size,unsigned char* _temp, int _decrpyt_requests)
{
        start = _start;
        finish = _finish;
        rpc = _rpc;
        ocall_freq = _ocall_freq;
        request_buffer = (unsigned char*) _request_buffer;
        request_buffer_size = _request_buffer_size;
        decrypted_request_buffer = (unsigned char*) malloc(request_buffer_size);
        temp = _temp;
        decrpyt_requests = _decrpyt_requests;

        hashmap = new HashMap();
//        for (size_t i=0;i<HASHMAPSIZE;i++)
//        {
//                item_t item;
//                item.key = i;
//                item.val = 0;
//                hashmap->add(item);
//        }
}

int test_llc_internal(long long times, int warmup)
{
//	printf("here\n");
        volatile long long res = 0;
        int res1;
        Aptr<char>* curr;
        int request_num = request_buffer_size / sizeof(long long);
        long long* request_keys = (long long*)request_buffer;

        for (long long x=0;x<times;x++)
        {
                if (x % ocall_freq == 0)
                {
                        if (rpc)
                        {
                                *finish = 1;
                                *start = 1 + warmup;
                                parameter_server_spin_lock(finish);
                        }
                        else
                        {
                                ocall_llc_test(&res1, warmup);
                                res += res1;
                        }
                }
                //for (int i=0;i<HASHMAPSIZE;i++)
                for (int i=0;i<request_num;i++)
                {

                        long long r = request_keys[i];
                        hashmap->get(r);
                }
        }

        return res;
}

int ecall_test_llc_misses_warmup()
{
        return test_llc_internal(100,1);
}

int ecall_test_llc_misses_real()
{
        return test_llc_internal(AVERAGE_TIMES,0);
}
