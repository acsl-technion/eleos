/*
 * Queue.h
 *
 *  Created on: Jun 24, 2016
 *      Author: user
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include "request.h"

class Queue {
public:
	Queue();
	virtual ~Queue();
	int enqueue(request* elem);
	request* dequeue();

private:
	static const int queue_size = 1024;
	int front, rear;
	request* q[queue_size];
	int volatile _lock;
};

#endif /* QUEUE_H_ */
