#ifndef _COMMON_STRUCTS_
#define _COMMON_STRUCTS

#include "../common/request.h"

struct s_server_data {
	void* _buffer;
	void* _client_data;
	request* _response;
};

#endif
