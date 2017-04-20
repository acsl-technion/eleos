/*
 * ocall_stdio.h
 *
 *  Created on: Jul 28, 2016
 *      Author: user
 */

#ifndef ENCLAVE_BZIP_TRUSTED_OCALL_STDIO_H_
#define ENCLAVE_BZIP_TRUSTED_OCALL_STDIO_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define O_RDONLY	     00
#define O_WRONLY	     01
#define O_RDWR		     02

int open(const char* filename, int mode);
int read(int file, void *buf, unsigned int size);
int write(int file, void *buf, unsigned int size);
void close(int file);
void printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif


#endif /* ENCLAVE_BZIP_TRUSTED_OCALL_STDIO_H_ */
