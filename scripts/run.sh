    #!/bin/bash

clear

# export LD_PRELOAD=/usr/local/lib/libmimalloc.so.1
# export MIMALLOC_SHOW_STATS=1
# export MIMALLOC_VERBOSE=1
# export MIMALLOC_SHOW_ERRORS=1

ulimit -c unlimited

if [ "$1" == "s" ]; then
    cd build/server
    rm -f server_log*.txt
    sudo ./hft_server
else [ "$1" == "t" ];
    cd build/trader
    rm -f trader_log*.txt
    sudo ./hft_trader
fi
