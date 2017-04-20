/*
* Queue.h
*
*  Created on: Jun 24, 2016
*      Author: user
*/

#ifndef QUEUE_H_
#define QUEUE_H_

#include "face_verification.h"

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
	unsigned int volatile _lock;
};

#endif /* QUEUE_H_ */