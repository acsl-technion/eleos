/*
 * my_types_u.h
 *
 *  Created on: May 11, 2016
 *      Author: user
 */

#ifndef ENCLAVE_SAMPLE_MY_TYPES_U_H_
#define ENCLAVE_SAMPLE_MY_TYPES_U_H_


#include <time.h>
#include <sys/stat.h>

typedef struct timezone *__restrict __timezone_ptr_t;
typedef unsigned long int __ino_t;
typedef __ino_t ino_t;
typedef unsigned int __uid_t;
typedef __uid_t uid_t;
typedef unsigned int gid_t;
typedef unsigned int __gid_t;
typedef unsigned int __mode_t;
typedef __mode_t mode_t;
typedef unsigned long int __dev_t;
typedef __dev_t dev_t;
typedef long int __off_t;
typedef int __pid_t;
typedef __pid_t pid_t;
typedef unsigned long int __syscall_ulong_t;
typedef long int __syscall_slong_t;
typedef long unsigned int size_t;
typedef long int ssize_t;
typedef long int __blksize_t;
typedef long int __blkcnt_t;
typedef unsigned long int __nlink_t;
typedef long int __suseconds_t;
typedef long int __time_t;
typedef __time_t time_t;

#endif /* ENCLAVE_SAMPLE_MY_TYPES_U_H_ */

