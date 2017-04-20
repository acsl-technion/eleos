#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */

#include "samples.h"
#include "samples_t.h"  /* print_string */
#include "../TestsEnum.h"
#include "face_verification/face_verification.h"

/* 
 * printf: 
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
void printf(const char *fmt, ...)
{
	char buf[BUFSIZ] = {'\0'};
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZ, fmt, ap);
	va_end(ap);
	ocall_samples_sample(buf);
}

int ecall_samples_sample()
{
	printf("IN SAMPLES\n");
	return 0;
}

int ecall_evaluate_lib(int testId, void* arg)
{
	TestsIds test = (TestsIds)testId;
	int fd = 0;
	long long numOfItems;

	switch (test)
	{
		default:
			printf("unkown option recieved");
			abort();
	}
	return 0;
}

int ecall_evaluate_regular(int testId, void* arg)
{
	TestsIds test = (TestsIds)testId;
	int numOfItems;
	switch (test)
	{
		case FaceVerification:
			//server is running here ----
			return workload_face_verification(arg);
			break;
		default:
			printf("unkown option recieved");
			abort();
	}
	return 0;
}

int ecall_evaluate_lib_preprocess(int testId, void* arg1, void* arg2)
{

	TestsIds test = (TestsIds)testId;
	int fd;
	long long numOfItems;

	switch (test)
	{
		default:
			printf("unkown option recieved");
			abort();
	}


	return 0;
}

int ecall_evaluate_regular_preprocess(int testId, void* arg1, void* arg2)
{
	TestsIds test = (TestsIds)testId;
	long long numOfItems;


	switch (test)
	{
		case FaceVerification:
			printf("Face Verification evaluate regular preprocess\n");
			face_verification_prepare_server(arg1, arg2);
			break;
		default:
			printf("unkown option recieved");
			abort();
	}


	return 0;
}
