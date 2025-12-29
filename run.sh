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
    sudo ./run_benchmarks --benchmark_color=yes --benchmark_filter=BM_Sys_ServerFix
    exit 0
fi

if [[ " ${args[@]} " =~ " ut " ]]; then
    cd build
    export GTEST_COLOR=1
    ctest --output-on-failure -V
    exit 0
fi

if [[ " ${args[@]} " =~ " it " ]]; then
    chmod +x scripts/setup_venv.sh
    ./scripts/setup_venv.sh
    ./tests/integration/it_run.sh "${args[@]:1}"
    exit 0
fi

if [[ " ${args[@]} " =~ " k " ]]; then
    # Check if Kafka is running
    if ! pgrep -f 'kafka.Kafka' > /dev/null; then
        echo "Starting Kafka"
        
        KAFKA_HOME="$HOME/src/kafka_2.13-4.0.0"
        KAFKA_LOG="$HOME/kafka.log"

        if [[ -x "$KAFKA_HOME/bin/kafka-server-start.sh" ]]; then
            nohup "$KAFKA_HOME/bin/kafka-server-start.sh" \
                "$KAFKA_HOME/config/server.properties" > "$KAFKA_LOG" 2>&1 &
            
            until nc -z localhost 9092; do
                echo -n "."
                sleep 1
            done
            echo "Kafka started, logging to $KAFKA_LOG"
        else
            echo "Kafka server script not found or not executable: $KAFKA_HOME/bin/kafka-server-start.sh"
            exit 1
        fi
    else
        echo "Kafka is already running."
    fi
fi

if [[ " ${args[@]} " =~ " s " ]]; then
    cd build/server
    rm -f server_log*.txt
    if [[ " ${args[@]} " =~ " val " ]]; then
        sudo -E valgrind --leak-check=full --show-leak-kinds=all --log-file=valgrind_output.log ./hft_server
    else
        sudo -E ./hft_server
    fi
elif [[ " ${args[@]} " =~ " c " ]]; then
    cd build/client
    rm -f client_log*.txt
    if [[ " ${args[@]} " =~ " val " ]]; then
        sudo -E valgrind --leak-check=full --show-leak-kinds=all --log-file=valgrind_output.log ./hft_client
    else
        sudo -E ./hft_client
    fi
elif [[ " ${args[@]} " =~ " m " ]]; then
    cd build/monitor
    rm -f monitor_log*.txt
    if [[ " ${args[@]} " =~ " val " ]]; then
        sudo -E valgrind --leak-check=full --show-leak-kinds=all --log-file=valgrind_output.log ./hft_monitor
    else
        sudo -E ./hft_monitor
    fi
else
    echo "Usage: $0 [k] [s|c|m|t|b] [val]"
    echo "k - run Kafka"
    echo "s - run server"
    echo "c - run client"
    echo "m - run monitor"
    echo "t - run tests"
    echo "b - run benchmarks"
    echo "val - run with valgrind"
fi
