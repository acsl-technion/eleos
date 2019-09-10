all: client gen_encrypted_dataset

client: face_detection_client.c
	g++ -O2 -o client face_detection_client.c -I. -I/opt/intel/sgxsdk/include -L. -L/opt/intel/sgxsdk/lib64 -lsgx_tcrypto -lpthread

gen_encrypted_dataset: generate_encrypted_dataset.c
	g++ -O2 -o gen_encrypted_dataset generate_encrypted_dataset.c -I. -I/opt/intel/sgxsdk/include -L. -L/opt/intel/sgxsdk/lib64 -lsgx_tcrypto -lpthread

clean:
	rm -f *.o client gen_encrypted_dataset
