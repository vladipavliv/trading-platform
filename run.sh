#!/bin/bash

clear

ulimit -c unlimited

./scripts/start_kafka.sh

if [ "$1" == "s" ]; then
    cd build/server
    rm -f server_log*.txt
    sudo ./hft_server
else [ "$1" == "c" ];
    cd build/client
    rm -f client_log*.txt
    sudo ./hft_client
fi
