#!/bin/bash
# SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$(pwd)/lib"
export LD_LIBRARY_PATH

cd `dirname $0` && pwd

./rpcfetch.sh &
cd build
sleep 1 # rpcpp tries to read the fifo too fast, need to wait before it's created

while true; do
    if [ "$(pgrep Discord)" = "" ]; then
        echo "discord not running yet, waiting"
        sleep 10
        continue
    fi
    ./rpcpp
    if [ $? == 137 ]; then
        break;
    fi
    sleep 5
    echo "Retrying"
done
pkill -x rpcfetch.sh
