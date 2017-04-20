/*
 * ocalls.cpp
 *
 *  Created on: Jul 13, 2016
 *      Author: user
 */

#include "ocalls.h"
#include "samples_t.h"

void* ocall_malloc(size_t size)
{
	void* retVal;
	ocall_malloc1(&retVal, size);
	return retVal;
}

void ocall_free(void* ptr)
{
	ocall_free1(ptr);
}

void* ocall_realloc(void* ptr,size_t size)
{
	void* retVal;
	ocall_realloc1(&retVal, ptr, size);
	return retVal;
}

int fcntl (int __fd, int __cmd, ...)
{
	int retVal;
	ocall_fcntl(&retVal, __fd, __cmd);
	return retVal;
}

int fchmod (int __fd, __mode_t __mode)
{
	int retVal;
	ocall_fchmod(&retVal, __fd, __mode);
	return retVal;
}

int unlink (const char *__name)
{
	int retVal;
	ocall_unlink(&retVal, __name);
	return retVal;
}

int mkdir (const char *__path, __mode_t __mode)
{
	int retVal;
	ocall_mkdir(&retVal, __path, __mode);
	return retVal;
}

int rmdir (const char *__path)
{
	int retVal;
	ocall_rmdir(&retVal, __path);
	return retVal;
}

int fchown (int __fd, __uid_t __owner, __gid_t __group)
{
	int retVal;
	ocall_fchown(&retVal, __fd, __owner, __group);
	return retVal;
}

char* getcwd (char *__buf, size_t __size)
{
	char* retVal;
	ocall_getcwd(&retVal, __buf, __size);
	return retVal;
}

int ftruncate (int __fd, __off_t __length)
{
	int retVal;
	ocall_ftruncate(&retVal, __fd, __length);
	return retVal;
}

ssize_t read (int __fd, void *__buf, size_t __nbytes)
{
	ssize_t retVal;
	ocall_read(&retVal, __fd, __buf, __nbytes);
	return retVal;
}

__uid_t geteuid (void)
{
	__uid_t retVal;
	ocall_geteuid(&retVal);
	return retVal;
}

void* mmap (void *__addr, size_t __len, int __prot, int __flags, int __fd, __off_t __offset)
{
	void* retVal;
	ocall_mmap(&retVal, __addr, __len, __prot, __flags, __fd, __offset);
	return retVal;
}

int munmap (void *__addr, size_t __len)
{
	int retVal;
	ocall_munmap(&retVal, __addr, __len);
	return retVal;
}

ssize_t readlink (const char* __path, char* __buf, size_t __len)
{
	ssize_t retVal;
	ocall_readlink (&retVal, __path, __buf, __len);
	return retVal;
}

ssize_t write (int __fd, const void *__buf, size_t __n)
{
	ssize_t retVal;
	ocall_write(&retVal, __fd, __buf, __n);
	return retVal;
}

int stat (const char* __file, struct stat* __buf)
{
	int retVal;
	ocall_stat (&retVal, __file, __buf);
	return retVal;
}

int lstat (const char* __file, struct stat* __buf)
{
	int retVal;
	ocall_lstat(&retVal, __file, __buf);
	return retVal;
}

int fstat (int __fd, struct stat *__buf)
{
	int retVal;
	ocall_fstat(&retVal, __fd, __buf);
	return retVal;
}

int access (const char *__name, int __type)
{
	int retVal;
	ocall_access(&retVal, __name, __type);
	return retVal;
}

int gettimeofday (struct timeval* __tv, __timezone_ptr_t __tz)
{
	int retVal;
	ocall_gettimeofday (&retVal, __tv, __tz);
	return retVal;
}

time_t time (time_t *__timer)
{
	time_t retVal;
	ocall_time(&retVal, __timer);
	return retVal;
}

long int sysconf (int __name)
{
	long int retVal;
	ocall_sysconf(&retVal, __name);
	return retVal;
}

int close (int __fd)
{
	int retVal;
	ocall_close(&retVal, __fd);
	return retVal;
}

int dlclose (void *__handle)
{
	int retVal;
	ocall_dlclose (&retVal, __handle);
	return retVal;
}

void* dlopen (const char *__file, int __mode)
{
	void* retVal;
	ocall_dlopen(&retVal, __file, __mode);
	return retVal;
}
char* dlerror (void)
{
	char* retVal;
	ocall_dlerror(&retVal);
	return retVal;
}

char* getenv (const char *__name)
{
	char* retVal;
	ocall_getenv(&retVal, __name);
	return retVal;
}

int fsync (int __fd)
{
	int retVal;
	ocall_fsync (&retVal, __fd);
	return retVal;
}

__pid_t getpid (void)
{
	__pid_t retVal;
	ocall_getpid(&retVal);
	return retVal;
}
__off_t lseek (int __fd, __off_t __offset, int __whence)
{
	__off_t retVal;
	ocall_lseek(&retVal, __fd, __offset, __whence);
	return retVal;
}

unsigned int sleep (unsigned int __seconds)
{
	unsigned int retVal;
	ocall_sleep(&retVal, __seconds);
	return retVal;
}

void* dlsym (void * __handle, const char * __name)
{
	void* retVal;
	ocall_dlsym(&retVal, __handle, __name);
	return retVal;
}

void *mremap (void *__addr, size_t __old_len, size_t __new_len, int __flags, ...)
{
	void* retVal;
	ocall_mremap(&retVal, __addr, __old_len,__new_len,__flags);
	return retVal;
}

int S_ISLNK(__mode_t m)
{
	int retVal;
	ocall_sislnk(&retVal, m);
	return retVal;
}

int S_ISDIR(__mode_t m)
{
	int retVal;
	ocall_sisdir(&retVal, m);
	return retVal;
}

int utimes (const char *__file, const struct timeval __tvp[2])
{
	int retVal;
	ocall_utimes(&retVal, __file); // other parameter is always null
	return retVal;
}

int open(__const char *__file, int __oflag, ...)
{
	int retval;
	ocall_open(&retval, __file, __oflag);
	return retval;
}


