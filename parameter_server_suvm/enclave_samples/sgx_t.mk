######## SGX SDK Settings ########
SGX_SDK ?= /opt/intel/sgxsdk
SGX_MODE ?= HW
SGX_ARCH ?= x64
SGX_PRERELEASE ?= 1

ifeq ($(shell getconf LONG_BIT), 32)
	SGX_ARCH := x86
else ifeq ($(findstring -m32, $(CXXFLAGS)), -m32)
	SGX_ARCH := x86
endif

ifeq ($(SGX_ARCH), x86)
	SGX_COMMON_CFLAGS := -m32
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x86/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x86/sgx_edger8r
else
	SGX_COMMON_CFLAGS := -m64
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib64
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x64/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x64/sgx_edger8r
endif

ifeq ($(SGX_DEBUG), 1)
ifeq ($(SGX_PRERELEASE), 1)
$(error Cannot set SGX_DEBUG and SGX_PRERELEASE at the same time!!)
endif
endif

ifeq ($(SGX_DEBUG), 1)
        SGX_COMMON_CFLAGS += -O0 -g
else
        SGX_COMMON_CFLAGS += -O3 -flto -mavx -msse4 -maes
endif

ifneq ($(SGX_MODE), HW)
	Trts_Library_Name := sgx_trts_sim
	Service_Library_Name := sgx_tservice_sim
else
	Trts_Library_Name := sgx_trts
	Service_Library_Name := sgx_tservice
endif

Crypto_Library_Name := sgx_tcrypto

services_lib = ../../eleos_core/trustedlib_lib_services

Samples_Cpp_Files := $(wildcard trusted/*.cpp) $(wildcard trusted/microbenchmarks/*.cpp) $(wildcard trusted/SQlite/*.cpp) $(wildcard trusted/hashmap/*.cpp) $(wildcard trusted/baseline_STLhashmap/*.cpp)
Samples_C_Files := #$(wildcard trusted/SQlite/*.c)
Samples_Include_Paths := -IInclude -Itrusted -I$(SGX_SDK)/include -I$(SGX_SDK)/include/tlibc -I$(SGX_SDK)/include/libcxx -I$(services_lib)/static_trusted

Flags_Just_For_C := -Wno-implicit-function-declaration -std=c11 -DSQLITE_THREADSAFE=0 $(EPCPP_CACHE_SIZE)
Common_C_Cpp_Flags := $(SGX_COMMON_CFLAGS) -nostdinc -fvisibility=hidden -fpie -fstack-protector $(Samples_Include_Paths) -fno-builtin-printf -fPIC -I.
Samples_C_Flags := $(Flags_Just_For_C) $(Common_C_Cpp_Flags)
Samples_Cpp_Flags :=  $(Common_C_Cpp_Flags) -std=c++11 -nostdinc++ -fno-builtin-printf -fpermissive -I.

Samples_Cpp_Flags := $(Samples_Cpp_Flags)  -fno-builtin-printf -fPIC $(EPCPP_CACHE_SIZE)

Samples_Link_Flags := $(SGX_COMMON_CFLAGS) -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L$(SGX_LIBRARY_PATH) -L$(services_lib) \
	-Wl,--whole-archive -l$(Trts_Library_Name) -Wl,--no-whole-archive \
	-Wl,--whole-archive -l_services.trusted -Wl,--no-whole-archive \
	-Wl,--start-group -lsgx_tstdc -lsgx_tcxx -l$(Crypto_Library_Name) -l$(Service_Library_Name) -Wl,--end-group \
	-Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
	-Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
	-Wl,--defsym,__ImageBase=0 \
	-Wl,--version-script=trusted/samples.lds

Samples_Cpp_Objects := $(Samples_Cpp_Files:.cpp=.o)
Samples_C_Objects := $(Samples_C_Files:.c=.o)

ifeq ($(SGX_MODE), HW)
ifneq ($(SGX_DEBUG), 1)
ifneq ($(SGX_PRERELEASE), 1)
Build_Mode = HW_RELEASE
endif
endif
endif


.PHONY: all run

ifeq ($(Build_Mode), HW_RELEASE)
all: samples.so
	@echo "Build enclave samples.so  [$(Build_Mode)|$(SGX_ARCH)] success!"
	@echo
	@echo "*********************************************************************************************************************************************************"
	@echo "PLEASE NOTE: In this mode, please sign the samples.so first using Two Step Sign mechanism before you run the app to launch and access the enclave."
	@echo "*********************************************************************************************************************************************************"
	@echo 


else
all: samples.signed.so
endif

run: all
ifneq ($(Build_Mode), HW_RELEASE)
	@$(CURDIR)/app
	@echo "RUN  =>  app [$(SGX_MODE)|$(SGX_ARCH), OK]"
endif


######## samples Objects ########

trusted/samples_t.c: $(SGX_EDGER8R) ./trusted/samples.edl
	@cd ./trusted && $(SGX_EDGER8R) --trusted ../trusted/samples.edl --search-path ../trusted --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

trusted/samples_t.o: ./trusted/samples_t.c
	@$(CC) $(Samples_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

trusted/baseline_STLhashmap/%.o : trusted/baseline_STLhashmap/%.cpp
	        @$(CXX) $(Samples_Cpp_Flags) -c $< -o $@
		@echo "CXX  <=  $<"

trusted/microbenchmarks/%.o: trusted/microbenchmarks/%.cpp
	@$(CXX) $(Samples_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

trusted/hashmap/%.o: trusted/hashmap/%.cpp
	@$(CXX) $(Samples_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

trusted/SQlite/%.o: trusted/SQlite/%.cpp
	@$(CXX) $(Samples_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"
	
trusted/SQlite/%.o: trusted/SQlite/%.c
	@$(CC) $(Samples_C_Flags) -c $< -o $@
	@echo "CC  <=  $<"
	
trusted/%.o: trusted/%.cpp
	@$(CXX) $(Samples_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

trusted/%.o: trusted/%.c
	@$(CC) $(Samples_C_Flags) -c $< -o $@
	@echo "CC  <=  $<"

samples.so: trusted/samples_t.o $(Samples_C_Objects) $(Samples_Cpp_Objects)
	@$(CXX) $^ -o $@ $(Samples_Link_Flags)
	@echo "LINK =>  $@"

samples.signed.so: samples.so
	@$(SGX_ENCLAVE_SIGNER) sign -key trusted/samples_private.pem -enclave samples.so -out $@ -config trusted/samples.config.xml
	@echo "SIGN =>  $@"
clean:
	@rm -f samples.* trusted/samples_t.* $(Samples_Cpp_Objects) $(Samples_C_Objects)
