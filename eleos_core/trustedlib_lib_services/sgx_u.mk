######## SGX SDK Settings ########
SGX_SDK ?= /opt/intel/sgxsdk
SGX_MODE ?= HW
#SGX_MODE ?= SIM
SGX_ARCH ?= x64
UNTRUSTED_DIR=untrusted
SGX_PRERELEASE=1

ifeq ($(shell getconf LONG_BIT), 32)
	SGX_ARCH := x86
else ifeq ($(findstring -m32, $(CXXFLAGS)), -m32)
	SGX_ARCH := x86
endif

ifeq ($(SGX_ARCH), x86)
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib
	SGX_COMMON_CFLAGS := -m32
	SGX_EDGER8R := $(SGX_SDK)/bin/x86/sgx_edger8r
else
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib64
	SGX_COMMON_CFLAGS := -m64
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

######## App Settings ########

App_Include_Paths := -IInclude -I$(UNTRUSTED_DIR) -I$(SGX_SDK)/include

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

App_Cpp_Flags := $(App_C_Flags) -std=c++11 $(EPCPP_CACHE_SIZE)

Lib_services_Cpp_Files := $(wildcard common/*.cpp) $(wildcard untrusted/*.cpp)
Lib_services_Cpp_Objects := $(Lib_services_Cpp_Files:.cpp=.o)

App_Link_Flags := $(SGX_COMMON_CFLAGS) -L$(SGX_LIBRARY_PATH) -l$(Urts_Library_Name) -lpthread 

ifneq ($(SGX_MODE), HW)
	App_Link_Flags += -lsgx_uae_service_sim
else
	App_Link_Flags += -lsgx_uae_service
endif

.PHONY: all run

all: lib_services.untrusted.a

######## App Objects ########

$(UNTRUSTED_DIR)/lib_services_u.c: $(SGX_EDGER8R) static_trusted/lib_services.edl
	@mkdir -p $(UNTRUSTED_DIR)
	@cd $(UNTRUSTED_DIR) && $(SGX_EDGER8R) --untrusted ../static_trusted/lib_services.edl --search-path ../static_trusted --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

$(UNTRUSTED_DIR)/lib_services_u.o: $(UNTRUSTED_DIR)/lib_services_u.c
	@$(CC) $(App_C_Flags) -c $< -o $@ $(App_Link_Flags)
	@echo "CC   <=  $<" 
	
common/%.o: common/%.cpp
	@$(CXX) $(App_Cpp_Flags) -c $< -o $@ $(App_Link_Flags)
	@echo "CXX  <=  $<"	
	
untrusted/%.o: untrusted/%.cpp
	@$(CXX) $(App_Cpp_Flags) -c $< -o $@ $(App_Link_Flags)
	@echo "CXX  <=  $<"
	
lib_services.untrusted.a: $(UNTRUSTED_DIR)/lib_services_u.o $(Lib_services_Cpp_Objects)
	ar rcs lib_services.untrusted.a $(Lib_services_Cpp_Objects) $(UNTRUSTED_DIR)/lib_services_u.o  
	@echo "LINK =>  $@"


.PHONY: clean

clean:
	@rm -f  $(UNTRUSTED_DIR)/lib_services_u.* $(UNTRUSTED_DIR)/*.o
