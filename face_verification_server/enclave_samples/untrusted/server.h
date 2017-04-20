#ifndef __SERVER_H_
#define __SERVER_H_

int runServer(int (*logic_func)(unsigned char* data, void* client_data),
              void* (*check_answer_func)(bool* is_empty, int* answer));

#endif
