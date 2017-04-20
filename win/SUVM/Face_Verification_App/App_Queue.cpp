#pragma once

#include "../Face_Verification_Enclave/Queue.h"
#include <stdlib.h>
#include <intrin.h>
#include <mmintrin.h>

unsigned int spin_lock(volatile unsigned int *lock)
{
	while (_InterlockedExchange8((volatile char*)lock, 1) != 0) {
		while (*lock) {
			/* tell cpu we are spinning */
			_mm_pause();
		}
	}

	return (0);
}

unsigned int spin_unlock(volatile unsigned int *lock)
{
	*lock = 0;

	return (0);
}


Queue::Queue()
{
	front = rear = 0;
	for (int i = 0; i<queue_size; i++)
	{
		q[i] = nullptr;
	}
}

Queue::~Queue()
{
}

int Queue::enqueue(request* elem)
{
	spin_lock(&_lock);

	if (rear - front == queue_size)
	{
		spin_unlock(&_lock);
		abort();
		return -1;
	}

	q[rear % queue_size] = elem;
	++rear;

	spin_unlock(&_lock);
	return 0;
}

request* Queue::dequeue()
{
	spin_lock(&_lock);

	if (front == rear)
	{
		spin_unlock(&_lock);
		return NULL;
	}

	request* result = q[front % queue_size];
	++front;

	spin_unlock(&_lock);

	return result;
}