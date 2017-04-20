/*
 * mem.hpp
 *
 *  Created on: Jun 23, 2016
 *      Author: user
 */

#ifndef TRUSTEDLIB_LIB_SERVICES_STATIC_TRUSTED_MEM_H_
#define TRUSTEDLIB_LIB_SERVICES_STATIC_TRUSTED_MEM_H_

#include "stdlib.h"
#include  <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

int mem_initialize(void* ptr, size_t size, int mnReq);
int memsys5Init(void *NotUsed, void* ptr, size_t p_size, int mnReq);
void memsys5Free(void *pOld);
void *memsys5Malloc(int nByte);
int memsys5Roundup(int n);
int memsys5Size(void *p);

#ifdef __cplusplus
}
#endif


#endif /* TRUSTEDLIB_LIB_SERVICES_STATIC_TRUSTED_MEM_H_ */
