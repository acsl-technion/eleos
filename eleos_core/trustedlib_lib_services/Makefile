
EPCPP_CACHE_SIZE ?= "-DCACHE_SIZE=67108864"
#RANDOM_ACCESS ?= "-DAPTR_RANDOM_ACCESS"
export EPCPP_CACHE_SIZE
#export RANDOM_ACCESS
all:
	@echo  "*************************************************************"
	@echo  "*****************************Cache size: " $(EPCPP_CACHE_SIZE) 
	@echo  "*************************************************************"
	$(MAKE) -f sgx_u.mk all
	$(MAKE) -f sgx_t_static.mk all

clean:
	$(MAKE) -f sgx_u.mk clean
	$(MAKE) -f sgx_t_static.mk clean

