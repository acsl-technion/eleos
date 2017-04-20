/*
 * Ocalls.cpp
 *
 *  Created on: Jun 11, 2016
 *      Author: user
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <pwd.h>
#include <libgen.h>
#include <stdlib.h>

#include <sgx_urts.h>

#include <dlfcn.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <time.h>

#include <dirent.h>

#include <pthread.h>

#include "samples_u.h"
#include "server.h"
#include <sys/socket.h>
#include <netinet/in.h>

void ocall_dummy()
{
}

int ocall_setup_server(int port)
{
/*
        int listenfd = 0;
        struct sockaddr_in serv_addr;

        listenfd = socket(AF_INET, SOCK_STREAM, 0);
        memset(&serv_addr, '0', sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(port);

        bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

        listen(listenfd, 128);

	return(listenfd);
*/
}

int ocall_accept(int sockfd)
{
	return (accept(sockfd, (struct sockaddr*)NULL, NULL)); 
}

void* ocall_readdir(void* dirp)
{
        struct dirent *dent;
        dent = readdir((DIR*)dirp);
        return ((void*)dent);
}

void* ocall_opendir(const char* name)
{
        DIR* d = opendir(name);
        return (void*)d;
}

int ocall_pavel_test(int i)
{
        printf("ocall_pavel_test i=%d\n", i);
        return(i*2);
}

int ocall_fchmod (int __fd, __mode_t __mode)
{
	printf("ocall36\n");
	return fchmod(__fd, __mode);
}

int ocall_unlink (const char *__name)
{
	printf("ocall35\n");
	return unlink(__name);
}

int ocall_mkdir (const char *__path, __mode_t __mode)
{
	printf("ocall34\n");
	return mkdir(__path, __mode);
}

int ocall_rmdir (const char *__path)
{
	printf("ocall33\n");
	return rmdir(__path);
}

int ocall_fchown (int __fd, __uid_t __owner, __gid_t __group)
{
	printf("ocall32\n");
	return fchown(__fd, __owner, __group);
}

char* ocall_getcwd (char *__buf, size_t __size)
{
	printf("ocall31\n");
	return getcwd(__buf, __size);
}

int ocall_ftruncate (int __fd, __off_t __length)
{
	printf("ocall30\n");
	return ftruncate(__fd, __length);
}

ssize_t ocall_read (int __fd, void *__buf, size_t __nbytes)
{
	//printf("ocall29\n");
	//printf("read %ld bytes\n", __nbytes);
	return read(__fd, __buf, __nbytes);
}

__uid_t ocall_geteuid (void)
{
	printf("ocall28\n");
	return geteuid();
}

void* ocall_mmap (void *__addr, size_t __len, int __prot, int __flags, int __fd, __off_t __offset)
{
	printf("ocall27\n");
	return mmap(__addr,__len, __prot, __flags, __fd,__offset);
}

int ocall_munmap (void *__addr, size_t __len)
{
	printf("ocall26\n");
	return munmap(__addr, __len);
}

ssize_t ocall_readlink (const char* __path, char* __buf, size_t __len)
{
	printf("ocall25\n");
	return readlink(__path, __buf, __len);
}

ssize_t ocall_write (int __fd, const void *__buf, size_t __n)
{
	//printf("ocall24\n");
	return write(__fd, __buf, __n);
}

int ocall_stat (const char* __file, struct stat* __buf)
{
	//printf("ocall23\n");
	return stat(__file, __buf);
}

int ocall_lstat (const char* __file, struct stat* __buf)
{
	printf("ocall22\n");
	int retval =lstat(__file, __buf);
	//printf("%d %s\n", retval, __file);
	return retval;
}

int ocall_fstat (int __fd, struct stat *__buf)
{
	//printf("ocall21\n");
	return fstat(__fd, __buf);
}

void *ocall_mremap (void *__addr, size_t __old_len, size_t __new_len, int __flags)
{
	printf("ocall20\n");
	return mremap(__addr, __old_len, __new_len, __flags);
}

int ocall_dlclose (void *__handle)
{
	printf("ocall19\n");
	return dlclose(__handle);
}

void* ocall_dlopen (const char *__file, int __mode)
{
	printf("ocall18\n");
	return dlopen(__file, __mode);
}

char* ocall_dlerror (void)
{
	printf("ocall17\n");
	return dlerror();
}

char* ocall_getenv (const char *__name)
{
	printf("ocall16\n");
	return getenv(__name);
}

int ocall_fsync (int __fd)
{
	printf("ocall15\n");
	return fsync(__fd);
}

__pid_t ocall_getpid (void)
{
	printf("ocall14\n");
	return getpid();
}

__off_t ocall_lseek (int __fd, __off_t __offset, int __whence)
{
	//printf("ocall13\n");
	return lseek(__fd, __offset, __whence);
}

int ocall_gettimeofday (struct timeval* __tv, __timezone_ptr_t __tz)
{
	printf("ocall12\n");
	return gettimeofday(__tv, __tz);
}

time_t ocall_time (time_t *__timer)
{
	printf("ocall11\n");
	return time(__timer);
}

int ocall_access (const char *__name, int __type)
{
	printf("ocall10\n");
	return access(__name, __type);
}

long int ocall_sysconf (int __name)
{
	printf("ocall9\n");
	return sysconf(__name);
}

unsigned int ocall_sleep (unsigned int __seconds)
{
	printf("ocall8\n");
	return sleep(__seconds);
}


int ocall_close (int __fd)
{
	//printf("ocall7\n");
	return close(__fd);
}

int ocall_fcntl (int __fd, int __cmd)
{
	//printf("ocall6\n");
	//int retval = fcntl(__fd, __cmd, tmp);
	//printf("retval: %d, cmd: %d, fd: %d\n", retval, __cmd, __fd);
	return 0;
}

void* ocall_dlsym(void * __handle, const char * __name)
{
	printf("ocall5\n");
	return dlsym(__handle, __name);
}

int ocall_utimes(const char *__file)
{
	printf("ocall4\n");
	return utimes(__file, NULL);
}

int ocall_sislnk(__mode_t m)
{
	printf("ocall3\n");
	return S_ISLNK(m);
}

int ocall_sisdir(__mode_t m)
{
	printf("ocall2\n");
	return S_ISDIR(m);
}

int ocall_open(const char *__file, int __oflag)
{
	//printf("ocall1\n");
	return open(__file, __oflag);
}

int ocall_open1(const char *__file, int __oflag, mode_t mode)
{
	//printf("ocall0\n");
	//printf("open %s\n", __file);
	return open(__file, __oflag, mode);
}

void* ocall_malloc1(size_t size)
{
	printf("malloc\n");
	return malloc(size);
}

void ocall_free1(void* ptr)
{
	printf("free\n");
	return free(ptr);
}

void* ocall_realloc1(void* ptr,size_t size)
{
	printf("realloc\n");
	return realloc(ptr, size);
}


