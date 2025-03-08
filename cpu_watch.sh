#!/bin/bash

if [ $# -eq 0 ]; then
    echo "Usage: $0 <cpu_index1> <cpu_index2> ..."
    exit 1
fi

#!/bin/bash
while true; do
    clear
    for i in "${@:-0 1 2}"; do
        freq_khz=$(cat /sys/devices/system/cpu/cpu$i/cpufreq/scaling_cur_freq)
        freq_ghz=$(echo "scale=2; $freq_khz / 1000000" | bc)
        printf "cpu $i: %0.2f GHz\n" "$freq_ghz"
    done
    sleep 1
done