#!/bin/bash

make EPCPP_CACHE_SIZE=-DCACHE_SIZE=67108864 clean ;  make EPCPP_CACHE_SIZE=-DCACHE_SIZE=62914560 # 60M

echo "200M"
#buffer size = 200M
./sample 1 1024 51200 1 0 3 100000 asdf > Logs7/200M_r1_aptr.log
./sample 1 1024 51200 1 0 2 100000 asdf > Logs7/200M_r1_reg.log
./sample 4 1024 51200 1 0 3 100000 asdf > Logs7/200M_r4_aptr.log
./sample 4 1024 51200 1 0 2 100000 asdf > Logs7/200M_r4_reg.log
