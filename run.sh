#!/bin/bash

clear

ulimit -c unlimited

if ! ps aux | grep -v grep | grep -q 'kafka.Kafka'
then
    Open a new terminal and run Kafka in it
    echo "Starting kafka..."
    gnome-terminal -- bash -c "~/src/kafka_2.13-4.0.0/bin/kafka-server-start.sh ~/src/kafka_2.13-4.0.0/config/server.properties; exec bash"
    sleep 10
fi

# sudo clickhouse start

if [ "$1" == "s" ]; then
    cd build/server
    rm -f server_log*.txt
    sudo ./hft_server
else [ "$1" == "c" ];
    cd build/client
    rm -f client_log*.txt
    sudo ./hft_client
fi
