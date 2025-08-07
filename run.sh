#!/bin/bash

ulimit -c unlimited

export POSTGRES_HOST=localhost
export POSTGRES_PORT=5432
export POSTGRES_USER=postgres
export POSTGRES_PASSWORD=password
export POSTGRES_DB=hft_db

args=("$@")

if [[ " ${args[@]} " =~ " b " ]]; then
    cd build/benchmarks
    sudo ./run_benchmarks --benchmark_color=yes
    exit 0
fi

if [[ " ${args[@]} " =~ " ut " ]]; then
    cd build
    export GTEST_COLOR=1
    ctest --output-on-failure
    exit 0
fi

if [[ " ${args[@]} " =~ " it " ]]; then
    chmod +x scripts/setup_venv.sh
    ./scripts/setup_venv.sh
    ./tests/integration/it_run.sh "${args[@]:1}"
    exit 0
fi

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
    sudo -E ./hft_server
elif [[ " ${args[@]} " =~ " c " ]]; then
    cd build/client
    rm -f client_log*.txt
    sudo -E ./hft_client
elif [[ " ${args[@]} " =~ " m " ]]; then
    cd build/monitor
    rm -f monitor_log*.txt
    sudo -E ./hft_monitor
else
    echo "Usage: $0 [k] [s|c|m|t|b]"
    echo "k - run Kafka"
    echo "s - run server"
    echo "c - run client"
    echo "m - run monitor"
    echo "t - run tests"
    echo "b - run benchmarks"
fi