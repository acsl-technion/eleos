/*
 * Aptr_tester.h
 *
 *  Created on: Jul 17, 2016
 *      Author: user
 */

#ifndef ENCLAVE_SAMPLES_TRUSTED_APTR_TESTER_H_
#define ENCLAVE_SAMPLES_TRUSTED_APTR_TESTER_H_

#include "Aptr.h"

int test_Aptr_create();
int test_Aptr_destroy();
int test_Aptr_dereference();
int test_Aptr_operator_add();
int test_Aptr_operator_sub();

int test_Aptr_double_dereference();
int test_Aptr_dereference_add_dereference_no_overflow();
int test_Aptr_dereference_add_dereference_with_overflow();
int test_Aptr_dereference_add_dereference_with_underflow();
int test_assignment_operation();
int test_evict_due_to_unlinked_state();

// TODO: add more tests

#endif /* ENCLAVE_SAMPLES_TRUSTED_APTR_TESTER_H_ */
