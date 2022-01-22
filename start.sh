#!/bin/sh
LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$(pwd)/lib"
export LD_LIBRARY_PATH

./rpcfetch.sh &
cd build
sleep 3 # rpcpp tries to read the fifo too fast, need to wait before it's created
./rpcpp
pkill -x rpcfetch.sh
