#pragma once

struct request
{
	int ocall_index;
	void* buffer;
	volatile int is_done;
};

struct s_server_data {
	void* _buffer;
	void* _client_data;
	request* _response;
};