/*
 * ocall_stdio.cpp
 *
 *  Created on: Jul 28, 2016
 *      Author: user
 */

#include "ocall_stdio.h"
#include <stdarg.h>
#include <string.h>
#include "samples_t.h"
#include <stdio.h>      /* vsnprintf */

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
    ocall_printf(buf);
}


int open(const char* filename, int mode) {
    int ret;
    if (ocall_open(&ret, filename, mode) != SGX_SUCCESS) return -1;
    return ret;
}

int read(int file, void *buf, unsigned int size) {
    int ret;
    if (ocall_read(&ret, file, buf, size) != SGX_SUCCESS) return -1;
    return ret;
}

int write(int file, void *buf, unsigned int size) {
	int ret;
    if (ocall_write(&ret, file, buf, size) != SGX_SUCCESS) return -1;
    return ret;
}

void close(int file) {
    ocall_close(file);
}
