/*
 * SyncUtils.cpp
 *
 *  Created on: Jun 23, 2016
 *      Author: user
 */

#include "SyncUtils.h"
#include <sgx_spinlock.h>

void spin_lock(unsigned int volatile*p)
{
	sgx_spin_lock(p);
}

void spin_unlock(unsigned int volatile*p)
{
	sgx_spin_unlock(p);
}

void spin_lock(unsigned char volatile *p)
{
	sgx_spin_lock((unsigned int volatile*)p);
}

void spin_unlock(unsigned char volatile *p)
{
	sgx_spin_unlock((unsigned int volatile*)p);
}