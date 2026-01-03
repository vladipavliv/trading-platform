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
2026-01-02T07:39:34+01:00
Running ./run_benchmarks
Run on (16 X 5231.35 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 98304 KiB (x1)
Load Average: 2.38, 0.88, 0.71
***WARNING*** ASLR is enabled, the results may have unreproducible noise in them.
--------------------------------------------------------------------------
Benchmark                                Time             CPU   Iterations
--------------------------------------------------------------------------
BM_Sys_ServerFix/AsyncProcess/1        110 ns          110 ns      8388608 1 worker(s)
BM_Sys_ServerFix/AsyncProcess/2       65.2 ns         65.2 ns     12582912 2 worker(s)
BM_Sys_ServerFix/AsyncProcess/3       51.2 ns         51.2 ns     14680064 3 worker(s)
BM_Sys_ServerFix/AsyncProcess/4       46.4 ns         46.4 ns     16777216 4 worker(s)
BM_Sys_OrderBookFix/AddOrder          43.8 ns         43.7 ns     15777792
BM_Op_LfqRunnerThroughput             15.6 ns         15.5 ns     46137344
BM_Op_LfqRunnerTailSpy                20.0 ns         20.0 ns     35651584 Max_ns=162.692 Min_ns=0 P50_ns=9.03846 P99.9_ns=63.2692 P99_ns=45.1923
BM_Op_StreamBusThroughput             20.0 ns         20.0 ns     35651584
```

Stress test (Server + Python tester with 5m pregenerated orders):
```bash
06:43:54.258145 [I] Orders: [opn|ttl] 810116|3711460 | Rps: 2875861
Sent 5000000 orders in 1.74s (2877129.41 orders/sec)
```

Manual localhost tests:
```bash
Server:
05:38:42.106367 [I] Orders: [opn|ttl] 132532|604277 | Rps: 69551
Client:
05:38:41.386266 [I] Rtt: [<50us|>50us] 100.00% avg:6us 0%
```