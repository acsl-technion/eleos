/*
 * SyncUtils.hpp
 *
 *  Created on: Jun 23, 2016
 *      Author: user
 */

#ifndef ENCLAVE_FORK_TRUSTED_SYNCUTILS_HPP_
#define ENCLAVE_FORK_TRUSTED_SYNCUTILS_HPP_

void spin_lock(unsigned int volatile *p);
void spin_unlock(unsigned int volatile *p);
void spin_lock(unsigned char volatile *p);
void spin_unlock(unsigned char volatile *p);

#endif /* ENCLAVE_FORK_TRUSTED_SYNCUTILS_HPP_ */
