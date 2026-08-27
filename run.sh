#!/bin/bash

ulimit -c unlimited

export POSTGRES_HOST=localhost
export POSTGRES_PORT=5432
export POSTGRES_USER=postgres
export POSTGRES_PASSWORD=password
export POSTGRES_DB=hft_db

args=("$@")

grant_perms() {
    if [ -f "$1" ]; then
        sudo setcap 'cap_sys_nice,cap_ipc_lock=eip' "$1"
    fi
}

if [[ " ${args[@]} " =~ " b " ]]; then
    cd build/benchmarks
    grant_perms "./run_benchmarks"
    ./run_benchmarks --benchmark_color=yes --benchmark_filter=BM_GenericQueue
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

if [[ " ${args[@]} " =~ " s " ]]; then
    cd build/server
    rm -f server_log*.txt
    grant_perms "./hft_server"
    if [[ " ${args[@]} " =~ " val " ]]; then
        sudo -E valgrind --leak-check=full --show-leak-kinds=all --log-file=valgrind_output.log ./hft_server
    else
        ./hft_server
    fi
elif [[ " ${args[@]} " =~ " c " ]]; then
    cd build/client
    rm -f client_log*.txt
    grant_perms "./hft_client"
    if [[ " ${args[@]} " =~ " val " ]]; then
        sudo -E valgrind --leak-check=full --show-leak-kinds=all --log-file=valgrind_output.log ./hft_client
    else
        ./hft_client
    fi
elif [[ " ${args[@]} " =~ " m " ]]; then
    cd build/monitor
    rm -f monitor_log*.txt
    grant_perms "./hft_monitor"
    if [[ " ${args[@]} " =~ " val " ]]; then
        sudo -E valgrind --leak-check=full --show-leak-kinds=all --log-file=valgrind_output.log ./hft_monitor
    else
        ./hft_monitor
    fi
else
    echo "Usage: $0 [k] [s|c|m|ut|b] [val]"
fi
