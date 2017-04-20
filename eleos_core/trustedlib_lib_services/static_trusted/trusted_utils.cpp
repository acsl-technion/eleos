/*
 * trusted_utils.cpp
 *
 *  Created on: Aug 17, 2016
 *      Author: user
 */

#include "trusted_utils.h"
#include "lib_services_t.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>      /* vsnprintf */


void debug(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_debug(buf);
}
