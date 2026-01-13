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
Serialization: SBE, FlatBuffers  
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
2026-01-13T13:22:35+01:00
Running ./run_benchmarks
Run on (16 X 5271 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 98304 KiB (x1)
Load Average: 0.93, 0.52, 0.54
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
***WARNING*** ASLR is enabled, the results may have unreproducible noise in them.
------------------------------------------------------------------------
Benchmark                              Time             CPU   Iterations
------------------------------------------------------------------------
BM_Sys_ServerFix/Throughput/1       62.8 ns         62.8 ns     11272192 1 worker(s)
BM_Sys_ServerFix/Throughput/2       37.5 ns         37.5 ns     18874368 2 worker(s)
BM_Sys_ServerFix/Throughput/3       27.4 ns         27.4 ns     25690112 3 worker(s)
BM_Sys_ServerFix/Throughput/4       22.1 ns         22.1 ns     31719424 4 worker(s)
BM_Sys_ServerFix/Latency/1           151 ns          151 ns      4408122 1 worker(s)
BM_Sys_ServerFix/Latency/2           163 ns          163 ns      4190611 2 worker(s)
BM_Sys_ServerFix/Latency/3           150 ns          150 ns      4552097 3 worker(s)
BM_Sys_ServerFix/Latency/4           145 ns          145 ns      4755725 4 worker(s)
BM_Sys_OrderBookFix/AddOrder        42.0 ns         42.0 ns     16433152
BM_Op_LfqRunnerThroughput           19.9 ns         19.9 ns     35651584
BM_Op_LfqRunnerTailSpy              17.7 ns         17.7 ns     39845888 Max_ns=912.885 Min_ns=0 P50_ns=9.03846 P99.9_ns=18.0769 P99_ns=18.0769
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
12:30:06.993808 [I] Orders: 23,249,495|103,300,662 Rtt: [<1µs|>1µs] 99.37%(2.5M) avg:301ns | 0.63%(15.8K) avg:2µs | Max:46µs
```