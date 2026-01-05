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
2026-01-04T01:07:52+01:00
Running ./run_benchmarks
Run on (16 X 5271 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 98304 KiB (x1)
Load Average: 0.66, 0.86, 0.62
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
***WARNING*** ASLR is enabled, the results may have unreproducible noise in them.
--------------------------------------------------------------------------
Benchmark                                Time             CPU   Iterations
--------------------------------------------------------------------------
BM_Sys_ServerFix/AsyncProcess/1        104 ns          104 ns      6291456 1 worker(s)
BM_Sys_ServerFix/AsyncProcess/2       64.7 ns         64.7 ns     12582912 2 worker(s)
BM_Sys_ServerFix/AsyncProcess/3       51.6 ns         51.6 ns     14680064 3 worker(s)
BM_Sys_ServerFix/AsyncProcess/4       46.6 ns         46.6 ns     16777216 4 worker(s)
BM_Sys_OrderBookFix/AddOrder          43.1 ns         43.1 ns     15990784
BM_Op_LfqRunnerThroughput             16.7 ns         16.7 ns     41943040
BM_Op_LfqRunnerTailSpy                20.4 ns         20.4 ns     35651584 Max_ns=216.923 Min_ns=0 P50_ns=9.03846 P99.9_ns=72.1154 P99_ns=54.2308
BM_Op_StreamBusThroughput             20.1 ns         20.1 ns     35651584
```

Stress test (Server + Python tester with 5m pregenerated orders):
```bash
02:16:10.878183 [I] Orders: [opn|ttl] 1815149|8323623 | Rps: 3195897
Sent 10000000 orders in 3.16s (3160574.80 orders/sec)
```

Manual localhost tests:
```bash
Server:
19:53:01.167132 [I] Orders: [opn|ttl] 3061144|14178017 | Rps: 471696
Client:
19:53:01.093317 [I] Rtt: [<50us|>50us] 99.99% avg:11us 0.01% avg:83us
```

Manual shared memory tests:
```bash
Server:
15:32:09.204617 [I] Orders: [opn|ttl] 11065605|51178954 | Rps: 3248902
Client:
15:32:08.457118 [I] Rtt: SizeTotal: 37813872 [<1us|>1us] 53.00% avg:628ns 47.00% avg:1us
```