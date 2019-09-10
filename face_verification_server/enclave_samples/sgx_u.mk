######## SGX SDK Settings ########
SGX_SDK ?= /opt/intel/sgxsdk
SGX_MODE ?= HW
SGX_ARCH ?= x64
UNTRUSTED_DIR=untrusted
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
        SGX_COMMON_CFLAGS += -O2
endif

######## App Settings ########

ifneq ($(SGX_MODE), HW)
	Urts_Library_Name := sgx_urts_sim
else
	Urts_Library_Name := sgx_urts
endif

unrusted_services_lib = ../../eleos_core/trustedlib_lib_services

# App_Cpp_Files := App/App.cpp $(wildcard App/Edger8rSyntax/*.cpp) $(wildcard App/TrustedLibrary/*.cpp)
App_Cpp_Files := $(UNTRUSTED_DIR)/sample.cpp $(UNTRUSTED_DIR)/Ocalls.cpp $(UNTRUSTED_DIR)/echoserver_threaded.cpp $(UNTRUSTED_DIR)/face_verification/face_verification.cpp # $(wildcard App/TrustedLibrary/*.cpp)
App_Include_Paths := -IInclude -I$(UNTRUSTED_DIR) -I$(SGX_SDK)/include -I$(unrusted_services_lib)/untrusted

App_C_Flags := $(SGX_COMMON_CFLAGS) -fPIC -Wno-attributes $(App_Include_Paths)

# Three configuration modes - Debug, prerelease, release
#   Debug - Macro DEBUG enabled.
#   Prerelease - Macro NDEBUG and EDEBUG enabled.
#   Release - Macro NDEBUG enabled.
ifeq ($(SGX_DEBUG), 1)
        App_C_Flags += -DDEBUG -UNDEBUG -UEDEBUG
else ifeq ($(SGX_PRERELEASE), 1)
        App_C_Flags += -DNDEBUG -DEDEBUG -UDEBUG
else
        App_C_Flags += -DNDEBUG -UEDEBUG -UDEBUG
endif

App_Cpp_Flags := $(App_C_Flags) -std=c++11 -fpermissive
App_Link_Flags := $(SGX_COMMON_CFLAGS) -L$(SGX_LIBRARY_PATH) -l$(Urts_Library_Name) -L$(unrusted_services_lib) -l_services.untrusted -levent -lpthread -lrt -ldl -L$(SGX_SDK)/lib64 -lsgx_tcrypto

ifneq ($(SGX_MODE), HW)
	App_Link_Flags += -lsgx_uae_service_sim
else
	App_Link_Flags += -lsgx_uae_service
endif

App_Cpp_Objects := $(App_Cpp_Files:.cpp=.o)



ifeq ($(SGX_MODE), HW)
ifneq ($(SGX_DEBUG), 1)
ifneq ($(SGX_PRERELEASE), 1)
Build_Mode = HW_RELEASE
endif
endif
endif


.PHONY: all run

ifeq ($(Build_Mode), HW_RELEASE)
all: sample
	@echo "Build sample [$(Build_Mode)|$(SGX_ARCH)] success!"
	@echo
	@echo "*********************************************************************************************************************************************************"
	@echo "PLEASE NOTE: In this mode, please sign the samples.so first using Two Step Sign mechanism before you run the app to launch and access the enclave."
	@echo "*********************************************************************************************************************************************************"
	@echo


else
all: sample
endif

run: all
ifneq ($(Build_Mode), HW_RELEASE)
	@$(CURDIR)/sample
	@echo "RUN  =>  sample [$(SGX_MODE)|$(SGX_ARCH), OK]"
endif

######## App Objects ########

$(UNTRUSTED_DIR)/samples_u.c: $(SGX_EDGER8R) trusted/samples.edl
	@cd $(UNTRUSTED_DIR) && $(SGX_EDGER8R) --untrusted ../trusted/samples.edl --search-path ../trusted --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

$(UNTRUSTED_DIR)/samples_u.o: $(UNTRUSTED_DIR)/samples_u.c
	@$(CC) $(App_C_Flags) -c $< -o $@ $(App_Link_Flags)
	@echo "CC   <=  $<"

$(UNTRUSTED_DIR)/face_verification/%.o: $(UNTRUSTED_DIR)/face_verification/%.cpp
	@$(CXX) $(App_Cpp_Flags) -c $< -o $@ $(App_Link_Flags)
	@echo "CXX  <=  $<"

$(UNTRUSTED_DIR)/%.o: $(UNTRUSTED_DIR)/%.cpp
	@$(CXX) $(App_Cpp_Flags) -c $< -o $@ $(App_Link_Flags)
	@echo "CXX  <=  $<"

sample: $(UNTRUSTED_DIR)/samples_u.o $(App_Cpp_Objects)
	@$(CXX) $^ -o $@ $(App_Link_Flags)
	@echo "LINK =>  $@"


.PHONY: clean

clean:
	@rm -f sample  $(App_Cpp_Objects) $(UNTRUSTED_DIR)/samples_u.* 
