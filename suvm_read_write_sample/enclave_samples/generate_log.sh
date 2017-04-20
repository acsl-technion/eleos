#!/bin/bash

make EPCPP_CACHE_SIZE=-DCACHE_SIZE=67108864 clean ;  make EPCPP_CACHE_SIZE=-DCACHE_SIZE=62914560 # 60M

echo "200M"
#buffer size = 200M
./sample 1 1024 51200 1 0 3 100000 asdf > Logs2/200M_r1_aptr.log
./sample 1 1024 51200 0 1 3 100000 asdf > Logs2/200M_w1_aptr.log
./sample 1 1024 51200 1 0 2 100000 asdf > Logs2/200M_r1_reg.log
./sample 1 1024 51200 0 1 2 100000 asdf > Logs2/200M_w1_reg.log
./sample 4 1024 51200 1 0 3 100000 asdf > Logs2/200M_r4_aptr.log
./sample 4 1024 51200 0 1 3 100000 asdf > Logs2/200M_w4_aptr.log
./sample 4 1024 51200 1 0 2 100000 asdf > Logs2/200M_r4_reg.log
./sample 4 1024 51200 0 1 2 100000 asdf > Logs2/200M_w4_reg.log

echo "500M"
#buffer size = 500M
./sample 1 1024 128000 1 0 3 100000 asdf > Logs2/500M_r1_aptr.log
./sample 1 1024 128000 0 1 3 100000 asdf > Logs2/500M_w1_aptr.log
./sample 1 1024 128000 1 0 2 100000 asdf > Logs2/500M_r1_reg.log
./sample 1 1024 128000 0 1 2 100000 asdf > Logs2/500M_w1_reg.log
./sample 4 1024 128000 1 0 3 100000 asdf > Logs2/500M_r4_aptr.log
./sample 4 1024 128000 0 1 3 100000 asdf > Logs2/500M_w4_aptr.log
./sample 4 1024 128000 1 0 2 100000 asdf > Logs2/500M_r4_reg.log
./sample 4 1024 128000 0 1 2 100000 asdf > Logs2/500M_w4_reg.log

echo "1G"
#buffer size = 1.0G
./sample 1 1024 262144 1 0 3 100000 asdf > Logs2/1.0G_r1_aptr.log
./sample 1 1024 262144 0 1 3 100000 asdf > Logs2/1.0G_w1_aptr.log
./sample 1 1024 262144 1 0 2 100000 asdf > Logs2/1.0G_r1_reg.log
./sample 1 1024 262144 0 1 2 100000 asdf > Logs2/1.0G_w1_reg.log
./sample 4 1024 262144 1 0 3 100000 asdf > Logs2/1.0G_r4_aptr.log
./sample 4 1024 262144 0 1 3 100000 asdf > Logs2/1.0G_w4_aptr.log
./sample 4 1024 262144 1 0 2 100000 asdf > Logs2/1.0G_r4_reg.log
./sample 4 1024 262144 0 1 2 100000 asdf > Logs2/1.0G_w4_reg.log

echo "1.5G"
#buffer size = 1.5G
./sample 1 1024 393216 1 0 3 100000 asdf > Logs2/1.5G_r1_aptr.log
./sample 1 1024 393216 0 1 3 100000 asdf > Logs2/1.5G_w1_aptr.log
./sample 1 1024 393216 1 0 2 100000 asdf > Logs2/1.5G_r1_reg.log
./sample 1 1024 393216 0 1 2 100000 asdf > Logs2/1.5G_w1_reg.log
./sample 4 1024 393216 1 0 3 100000 asdf > Logs2/1.5G_r4_aptr.log
./sample 4 1024 393216 0 1 3 100000 asdf > Logs2/1.5G_w4_aptr.log
./sample 4 1024 393216 1 0 2 100000 asdf > Logs2/1.5G_r4_reg.log
./sample 4 1024 393216 0 1 2 100000 asdf > Logs2/1.5G_w4_reg.log
