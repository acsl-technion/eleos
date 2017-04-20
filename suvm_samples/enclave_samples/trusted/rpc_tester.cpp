/*
 * rpc_tester.cpp
 *
 *  Created on: Aug 16, 2016
 *      Author: user
 */

#include "rpc_tester.h"
#include <ocall_stdio.h>

#define TEST_FILE_NAME "/home/user/test.db"
#define BUF_SIZE 256

int test_open_close()
{
	int res = true;

	int fd = open(TEST_FILE_NAME, O_RDONLY);
	res &= fd != -1;
	close(fd);

	return res;
}
int test_open_read_close()
{
	int res = true;

	int fd = open(TEST_FILE_NAME, O_RDONLY);
	res &= fd != -1;
	char buf[BUF_SIZE];
	int bytes = read(fd, buf, BUF_SIZE);
	res &= bytes == BUF_SIZE;

	close(fd);

	return res;

}
int test_open_write_close()
{
	int res = true;

	int fd = open(TEST_FILE_NAME, O_RDWR);
	res &= fd != -1;
	char buf[BUF_SIZE];
	int bytes = write(fd, buf, BUF_SIZE);
	res &= bytes == BUF_SIZE;

	close(fd);

	return res;
}
int test_open_read_write_close()
{
	int res = true;

	int fd = open(TEST_FILE_NAME, O_RDWR);
	res &= fd != -1;
	char buf[BUF_SIZE];
	int bytes_read = read(fd, buf, BUF_SIZE);
	res &= bytes_read == BUF_SIZE;

	int bytes_write = write(fd, buf, BUF_SIZE);
	res &= bytes_write == BUF_SIZE;

	// TODO: validate for non-multi-threaded case

	close(fd);

	return res;
}
