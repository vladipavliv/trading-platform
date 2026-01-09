# Trading platform

## Table of contents
- [Introduction](#introduction)
- [Installation](#installation)
- [Usage](#usage)
    - [Setup environment](#setup-environment)
    - [Configuration](#configuration)
    - [Run](#run)
    - [Commands](#commands)
- [Performance](#performance)

## Introduction
C++ trading platform built on Boost.Asio, featuring a lock-free, template-based distributed system with real-time Kafka telemetry.

## Installation

### Main dependencies
Core libraries: Boost, Folly  
Serialization: SBE, FlatBuffers, Protobuf  
Database: libpqxx  
Messaging: librdkafka  
Other: spdlog

### Setup repository
```bash
git clone https://github.com/vladipavliv/trading-platform.git
cd trading-platform
./build.sh [options]

Options:
  c   - clean
  d   - debug
  t   - include tests
  b   - include benchmarks
  tel - enable telemetry via kafka
```

## Usage
### Setup environment
Install Postgres, Kafka (if you want to enable telemetry monitoring via separate service) and ClickHouse (if you want to persist telemetry). Set them up with the scripts
```bash
python3 ./scripts/setup_postgres.py
python3 ./scripts/setup_kafka.py
./scripts/setup_clickhouse.sh
```

### Configuration
Client and server use separate config files to setup url, ports, core ids for network, application, and system threads, log file name and other settings.
Adjust the settings in 

```bash
build/server/server_config.ini
build/client/client_config.ini
build/monitor/monitor_config.ini
```

### Run
```bash
./run.sh <component> [k]

Components:
  s   - server
  c   - client
  m   - monitor
  ut  - unit tests
  it  - integration tests
  b   - benchmarks

Optional flag:
  k   - start Kafka
```

### Commands
```bash
Server:
  p+  - start broadcasting price updates
  p-  - stop broadcasting price updates
  t+  - start telemetry streaming
  t-  - stop telemetry streaming
  q   - shutdown

Client:
  s+  - start trader
  s-  - stop trader
  t+  - start telemetry streaming
  t-  - stop telemetry streaming
  q   - shutdown

Monitor:
  Supports all client and server commands
```

## Performance

Benchmarks:
```bash
2026-01-09T20:49:52+01:00
Running ./run_benchmarks
Run on (16 X 5271 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 98304 KiB (x1)
Load Average: 1.01, 0.63, 0.65
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
***WARNING*** ASLR is enabled, the results may have unreproducible noise in them.
--------------------------------------------------------------------------
Benchmark                                Time             CPU   Iterations
--------------------------------------------------------------------------
BM_Sys_ServerFix/AsyncProcess/1       70.1 ns         67.9 ns     10485760 1 worker(s)
BM_Sys_ServerFix/AsyncProcess/2       38.9 ns         36.7 ns     20971520 2 worker(s)
BM_Sys_ServerFix/AsyncProcess/3       26.6 ns         24.3 ns     29360128 3 worker(s)
BM_Sys_ServerFix/AsyncProcess/4       20.6 ns         18.2 ns     39845888 4 worker(s)
BM_Sys_OrderBookFix/AddOrder          43.1 ns         43.1 ns     15990784
BM_Op_LfqRunnerThroughput             16.7 ns         16.7 ns     41943040
BM_Op_LfqRunnerTailSpy                20.4 ns         20.4 ns     35651584 Max_ns=216.923 Min_ns=0 P50_ns=9.03846 P99.9_ns=72.1154 P99_ns=54.2308
BM_Op_StreamBusThroughput             20.1 ns         20.1 ns     35651584
```

Manual localhost tests:
```bash
Server:
16:30:07.122596 [I] Orders: [opn|ttl] 4,928,088|22,803,781 | Rps: 529,529
Client:
16:30:07.845325 [I] Rtt: [<10µs|>10µs] 18.39%(75.8K) avg:8µs | 81.61%(336.2K) avg:25µs | Max:136µs
```

Manual shared memory tests:
```bash
Server:
15:59:10.350354 [I] Orders: [opn|ttl] 60,540,877|280,107,828 | Rps: 5,981,976
Client:
15:59:10.535558 [I] Rtt: [<1µs|<10µs|>10µs] 82.79%(3.8M) avg:525ns | 17.15%(795.9K) avg:1µs | 0.06%(2.8K) avg:20µs | Max:36µs
```