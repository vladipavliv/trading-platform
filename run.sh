#!/bin/bash

clear

cd build 

if [ "$1" == "s" ] || [ "$1" == "server" ]; then
    clear
    sudo ./hft_server
elif [ "$1" == "t" ] || [ "$1" == "trader" ]; then
    clear
    sudo ./hft_trader
fi
