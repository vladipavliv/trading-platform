    #!/bin/bash

clear

# export LD_PRELOAD=/usr/local/lib/libmimalloc.so.1
# export MIMALLOC_SHOW_STATS=1
# export MIMALLOC_VERBOSE=1
# export MIMALLOC_SHOW_ERRORS=1

cd build 

ulimit -c unlimited

if [ "$1" == "s" ]; then
    rm -f server_log.*.txt
    sudo ./hft_server
elif [ "$1" == "t" ]; then
    rm -f trader_log.*.txt
    sudo ./hft_trader
else [ "$1" == "s2" ];
    rm -f server_log.*.txt
    sudo ./hft_server_v2
fi
