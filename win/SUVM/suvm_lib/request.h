/*
 * request.h
 *
 *  Created on: Jun 25, 2016
 *      Author: user
 */

#ifndef REQUEST_H_
#define REQUEST_H_

#include <stdlib.h>

struct request
{
	int ocall_index;
	void* buffer;
	volatile int is_done;
};

#endif
