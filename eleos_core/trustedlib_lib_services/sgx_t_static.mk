######## SGX SDK Settings ########
SGX_SDK ?= /opt/intel/sgxsdk
SGX_MODE ?= HW
#SGX_MODE ?= SIM
SGX_ARCH ?= x64
SGX_PRERELEASE=1

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
        SGX_COMMON_CFLAGS += -O3 -mavx -msse4 -maes
endif

ifneq ($(SGX_MODE), HW)
	Trts_Library_Name := sgx_trts_sim
	Service_Library_Name := sgx_tservice_sim
else
	Trts_Library_Name := sgx_trts
	Service_Library_Name := sgx_tservice
endif

Crypto_Library_Name := sgx_tcrypto

Lib_services_Cpp_Files := $(wildcard common/*.cpp) $(wildcard static_trusted/*.cpp)
#lib_services.cpp static_trusted/SyncUtils.cpp 
Lib_services_C_Files := 
Lib_services_Include_Paths := -IInclude -ILib_services -I$(SGX_SDK)/include -I$(SGX_SDK)/include/tlibc -I$(SGX_SDK)/include/stlport -I/usr/lib/gcc/x86_64-linux-gnu/4.8/include

Flags_Just_For_C := -Wno-implicit-function-declaration -std=c11
Common_C_Cpp_Flags := $(SGX_COMMON_CFLAGS) -nostdinc -fvisibility=hidden -fpie -fstack-protector $(Lib_services_Include_Paths) -fno-builtin-printf -fPIC -I. $(EPCPP_CACHE_SIZE) $(RANDOM_ACCESS)
Lib_services_C_Flags := $(Flags_Just_For_C) $(Common_C_Cpp_Flags)
Lib_services_Cpp_Flags := $(Common_C_Cpp_Flags) -std=c++11 -nostdinc++ -fno-builtin-printf  -I.

Lib_services_Cpp_Flags := $(Lib_services_Cpp_Flags)  -fno-builtin-printf -fPIC

Lib_services_Link_Flags := $(SGX_COMMON_CFLAGS) -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L$(SGX_LIBRARY_PATH) \
	-Wl,--whole-archive -l$(Trts_Library_Name) -Wl,--no-whole-archive \
	-Wl,--start-group -lsgx_tstdc -lsgx_tstdcxx -l$(Crypto_Library_Name) -l$(Service_Library_Name) -Wl,--end-group \
	-Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
	-Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
	-Wl,--defsym,__ImageBase=0 \
	-Wl,--version-script=trusted/lib_services.lds


Lib_services_Cpp_Objects := $(Lib_services_Cpp_Files:.cpp=.o)
Lib_services_C_Objects := $(Lib_services_C_Files:.c=.o)

ifeq ($(SGX_MODE), HW)
ifneq ($(SGX_DEBUG), 1)
ifneq ($(SGX_PRERELEASE), 1)
Build_Mode = HW_RELEASE
endif
endif
endif


.PHONY: all run

all: lib_services.trusted.a

######## lib_services Objects ########

static_trusted/lib_services_t.h: $(SGX_EDGER8R) ./static_trusted/lib_services.edl
	@cd ./static_trusted && $(SGX_EDGER8R) --header-only  --trusted ../static_trusted/lib_services.edl --search-path ../static_trusted --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

static_trusted/lib_services_t.o: ./trusted/lib_services_t.c
	@$(CC) $(Lib_services_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

common/%.o: common/%.cpp
	@$(CXX) $(Lib_services_Include_Paths) $(Lib_services_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

static_trusted/%.o: static_trusted/%.cpp
	@$(CXX) $(Lib_services_Include_Paths) $(Lib_services_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

common/%.o: common/%.c
	@$(CC) $(Lib_services_C_Flags) -c $< -o $@
	@echo "CC  <=  $<"

static_trusted/%.o: static_trusted/%.c
	@$(CC) $(Lib_services_C_Flags) -c $< -o $@
	@echo "CC  <=  $<"

lib_services.trusted.a: static_trusted/lib_services_t.h $(Lib_services_Cpp_Objects) $(Lib_services_C_Objects)
	ar rcs lib_services.trusted.a $(Lib_services_Cpp_Objects) $(Lib_services_C_Objects)  
	@echo "LINK =>  $@"

clean:
	@rm -f lib_services.* static_trusted/lib_services_t.* $(Lib_services_Cpp_Objects) $(Lib_services_C_Objects)
