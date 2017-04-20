#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
 
int main(int argc , char *argv[])
{
	if (argc != 5)
	{
		printf("Usage client [REQUEST_NUM] [USE_FILE] [File_PATH] [CACHE_SIZE(MB)]\n");
		return -1;
	}


    srand(time(NULL));
    int sock;
    struct sockaddr_in server;
    size_t req_size = atoi(argv[1]);
    long long* server_req = (long long*)malloc(req_size);
    const long long cache_size = atoi(argv[4])*1024 / (sizeof(long long) * 2);

    int use_file = atoi(argv[2]);
    
    int fd = open(argv[3], O_RDONLY);

    struct timespec ct1,ct2;
while(1) { 
    //Create socket	
    sleep(1);
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket\n");
    }
     
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons( 5002);
    while (1) { 
    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
	sleep(1);
	continue;
    }
     
    char server_rep[1];    
    lseek(fd, 0, SEEK_SET); 
    int counter = 0; 

 // Generate keys
            for (int i=0;i<req_size/sizeof(long long);i++)
            {
                 server_req[i] = rand() % cache_size;
            }


    //keep communicating with server
    while(1)
    {
	counter++;
        if (counter == 10)
        {
	 	// warmup done
		clock_gettime(CLOCK_REALTIME, &ct1);
        }

        //Send keys to the server
        if( send(sock , server_req, req_size, 0) <= 0)
        {
            break;
        }

		 // Generate keys
            for (int i=0;i<req_size/sizeof(long long);i++)
            {
                 server_req[i] = rand() % cache_size;
            }


	// wait for response
	int n;
	while ((n = recv(sock, server_rep, 1, 0)) <= 0)
	{
	}

	if (server_rep[0] == 'e')
	{
   	        clock_gettime(CLOCK_REALTIME, &ct2);
		break;
	}
    }

    shutdown(sock, SHUT_RDWR);
    close(sock);

    double latency = ( ct2.tv_sec - ct1.tv_sec ) + ( ct2.tv_nsec - ct1.tv_nsec ) / 1e9;
    printf("request: %ld: cache size: %d: latency: %lf\n", req_size, atoi(argv[4]), latency);

    break;
  }
}
     
    free(server_req);
    close(sock);
    close(fd);
    return 0;
}
