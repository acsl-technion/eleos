# Eleos
ExitLess Services for SGX Enclaves

Tested on Ubuntu 14.04 64 bit.

Prerequisites:
1. Intel SGX Driver: https://github.com/01org/linux-sgx-driver
2. Intel SGX SDK & PSW: https://github.com/01org/linux-sgx

For running memcached:
1. Graphene with Eleos RPC: Will be made available shortly  

Directory structure:
eleos_core - the exit-less services lib
suvm_samples - SUVM example usages
Edger - edger8r utility with RPC instead of OCALL integration
parameter_server - paramter server application from the paper
parameter_server_suvm - paramter server from the paper with SUVM support
client_parameter_server - workload generator for the parameter server from the paper 
face_verification_server - face verification workload from the paper
suvm_read_write_sample - suvm benchmarks from the paper

Stay tuned for further support for the Windows platform.


Find the full details here:
[EUROSYS17] "Eleos: Exit-Less OS Services for  SGX Enclaves" 
M. Orenbach, M. Minkin, P. Lifshits, M. Silberstei