#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */

#include "samples.h"
#include "samples_t.h"  /* print_string */

#include "mem_tester.h"
#include "Aptr_tester.h"
#include "rpc_tester.h"

int ecall_samples_sample(void* pool_ptr)
{
  printf("Performing Tests\n");
  printf("-----------------\n");

  bool total_res = true;
  bool res = false;
  // Mem allocation
  res = test_memsys5Malloc();
  total_res &= res;
  if (!res)
	  printf("test_memsys5Malloc: %s\n", res ? "PASSED" : "FAILED");
  res = test_memsys5Roundup();
  total_res &= res;
  if (!res)
	  printf("test_memsys5Roundup: %s\n", res ? "PASSED" : "FAILED");
  res = test_memsys5Size();
  total_res &= res;
  if (!res)
	  printf("test_memsys5Size: %s\n", res ? "PASSED" : "FAILED");
  res = test_page_alignment(pool_ptr);
  total_res &= res;
  if (!res)
	  printf("test_page_alignment: %s\n", res ? "PASSED" : "FAILED");
  
  // SUVM
  res = test_Aptr_create();
  total_res &= res;
  if (!res)
	  printf("test_Aptr_create: %s\n", res ? "PASSED" : "FAILED");

  res = test_Aptr_destroy();
  total_res &= res;
  if (!res)
  	  printf("test_Aptr_destroy: %s\n", res ? "PASSED" : "FAILED");

  res = test_Aptr_dereference();
  total_res &= res;
  if (!res)
	  printf("test_Aptr_dereference: %s\n", res ? "PASSED" : "FAILED");

  res = test_Aptr_operator_add();
  total_res &= res;
  if (!res)
  	 printf("test_Aptr_operator_add: %s\n", res ? "PASSED" : "FAILED");

  res = test_Aptr_operator_sub();
  total_res &= res;
  if (!res)
  	 printf("test_Aptr_operator_sub: %s\n", res ? "PASSED" : "FAILED");

  res = test_Aptr_double_dereference();
  total_res &= res;
  if (!res)
  	 printf("test_Aptr_double_dereference: %s\n", res ? "PASSED" : "FAILED");

  res = test_Aptr_dereference_add_dereference_no_overflow();
  total_res &= res;
  if (!res)
  	 printf("test_Aptr_dereference_add_dereference_no_overflow: %s\n", res ? "PASSED" : "FAILED");

  res = test_Aptr_dereference_add_dereference_with_overflow();
  total_res &= res;
  if (!res)
  	 printf("test_Aptr_dereference_add_dereference_with_overflow: %s\n", res ? "PASSED" : "FAILED");

  res = test_Aptr_dereference_add_dereference_with_underflow();
  total_res &= res;
  if (!res)
  	 printf("test_Aptr_dereference_add_dereference_with_underflow: %s\n", res ? "PASSED" : "FAILED");

  res = test_assignment_operation();
  total_res &= res;
  if (!res)
  	 printf("test_assignment_operation: %s\n", res ? "PASSED" : "FAILED");

  res = test_evict_due_to_unlinked_state();
  total_res &= res;
  if (!res)
  	 printf("test_evict_due_to_unlinked_state: %s\n", res ? "PASSED" : "FAILED");


  return !total_res;
}
