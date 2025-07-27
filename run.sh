#!/bin/bash

clear

ulimit -c unlimited

args=("$@")

if [[ " ${args[@]} " =~ " k " ]]; then
    if ! pgrep -f 'kafka.Kafka' > /dev/null; then
        echo "Starting Kafka..."
        gnome-terminal -- bash -c "~/src/kafka_2.13-4.0.0/bin/kafka-server-start.sh ~/src/kafka_2.13-4.0.0/config/server.properties; exec bash"
        sudo clickhouse start
        sleep 10
    else
        echo "Kafka is already running."
    fi
fi

if [[ " ${args[@]} " =~ " s " ]]; then
    cd build/server
    rm -f server_log*.txt
    sudo ./hft_server
elif [[ " ${args[@]} " =~ " c " ]]; then
    cd build/client
    rm -f client_log*.txt
    sudo ./hft_client
elif [[ " ${args[@]} " =~ " m " ]]; then
    cd build/monitor
    rm -f monitor_log*.txt
    sudo ./hft_monitor
else
    echo "Usage: $0 [k] [s|c|m]"
    echo "k - start Kafka"
    echo "s - start server"
    echo "c - start client"
    echo "m - start monitor"
fi