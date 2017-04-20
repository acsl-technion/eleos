#IPPCP_LIB_DIR=/home/user/ippcp_untrusted_lib/

all: client

client: face_detection_client.c
	g++ -O2 -o client face_detection_client.c -I. -I/opt/intel/sgxsdk/include -L$(IPPCP_LIB_DIR) -lcrypto -lpthread

clean:
	rm -f *.o client
