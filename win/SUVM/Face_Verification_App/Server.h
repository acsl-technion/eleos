#pragma once

int runServer(int(*logic_func)(unsigned char* data, void* client_data),
	void* (*check_answer_func)(bool* is_empty, int* answer));