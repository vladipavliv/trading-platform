#!/bin/bash

clear

# export LD_PRELOAD=/usr/local/lib/libmimalloc.so.1
# export MIMALLOC_SHOW_STATS=1
# export MIMALLOC_VERBOSE=1
# export MIMALLOC_SHOW_ERRORS=1

cd build 

ulimit -c unlimited

if [ "$1" == "s" ]; then
    sudo ./hft_server
else [ "$1" == "t" ] || [ "$1" == "trader" ];
    sudo ./hft_trader
fi
