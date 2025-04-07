#!/bin/bash

clear

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
