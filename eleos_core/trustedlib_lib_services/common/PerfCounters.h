#ifndef __PERF_COUNTERS
#define __PERF_COUNTERS


#define ASSERT(cond)\
    do\
    {\
        if (!(cond))\
        {\
            debug("ASSERTION FAILED: cond = %s, file = %s, line = %d, 0 = %d\n", #cond, __FILE__, __LINE__, 0);\
            abort();\
        }\
    } while(0)  




#define NUM_COUNTERS (10)
#define CLEAN_COUNTERS(P) {for(int _ccc=0;_ccc<NUM_COUNTERS;(P)[_ccc++]=0);}
#define PF_COUNT_INC(P) (P)[0]++;
#define ENCRYPT_COUNT_INC(P) (P)[1]++;
#define PF_COUNT_RESET(P) (P)[0]=0;
#define PF_COUNT(P) (P)[0]
#define ENCRYPT_COUNT(P) (P)[1]

// using counter 2 to measure PF cycles
#define BEGIN_PF_MEASURE(P) do {\
	(P)[2]++;\
	if(((P)[2])%2 != 1)\
		(P)[2]++;\
	ASSERT(((P)[2])%2 == 1);\
} while(0)

#define END_PF_MEASURE(P) do {\
	(P)[2]++;\
	if(((P)[2])%2 == 1) /* so the ending will never look as a beginning */\
		(P)[2]++;\
	ASSERT(((P)[2])%2 != 1);\
} while(0)

#define READ_COUNTER_2(P) (P)[2]
#define COUNTER_2_INC_5(P) (P)[2]+=5;



#endif
