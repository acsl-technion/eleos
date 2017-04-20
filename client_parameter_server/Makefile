#IPPCP_LIB_DIR=/home/user/ippcp_untrusted_lib/

all: client

client: client.cpp
	g++ -O2 -o client client.cpp -I. -lpthread

clean:
	rm -f *.o client
