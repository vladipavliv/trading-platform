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
2025-12-28T19:30:58+01:00
Running ./run_benchmarks
Run on (20 X 4600 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x10)
  L1 Instruction 32 KiB (x10)
  L2 Unified 1280 KiB (x10)
  L3 Unified 24576 KiB (x1)
Load Average: 1.60, 1.00, 0.40
------------------------------------------------------------------------------------------------
Benchmark                                      Time             CPU   Iterations UserCounters...
------------------------------------------------------------------------------------------------
BM_Sys_ServerFix/AsyncProcess_1Worker        227 ns          224 ns      2966545 items_per_second=4.47133M/s
BM_Sys_ServerFix/AsyncProcess_2Workers       273 ns          272 ns      2550539 items_per_second=3.68002M/s
BM_Sys_ServerFix/AsyncProcess_3Workers       362 ns          361 ns      1928000 items_per_second=2.76702M/s
BM_Sys_ServerFix/AsyncProcess_4Workers       509 ns          508 ns      1346816 items_per_second=1.96842M/s
BM_Ser_ProtoSerialize                        182 ns          182 ns      3736254
BM_Ser_ProtoDeserialize                      133 ns          133 ns      5200438
BM_Ser_FbsSerialize                          150 ns          150 ns      5359374
BM_Ser_FbsDeserialize                       31.6 ns         31.6 ns     21990438
BM_Ser_SbeSerialize                         2.43 ns         2.42 ns    288539113
BM_Ser_SbeDeserialize                       11.7 ns         11.7 ns     63787435
BM_Sys_OrderBookFix/AddOrder                99.9 ns         99.9 ns      6959726
BM_Op_VykovMpmcQueue                        14.1 ns         14.1 ns     49494243
BM_Op_FollyMpmcQueue                        48.5 ns         48.5 ns     14495449
BM_Op_BoostMpmcQueue                        36.0 ns         36.0 ns     19423840
BM_Op_MessageBusPost                        2.07 ns         2.07 ns    342791320
BM_Op_SystemBusPost                          240 ns          236 ns      3088693
```

Stress test (Server + Python tester with 5m pregenerated orders):
```bash
00:36:28.746653 [I] Orders: [opn|ttl] 986,963|4,528,793 | Rps: 1,866,237
[Client 0] Sent 5000000 orders in 2.70s (1852510.08 orders/sec)
```

Manual localhost tests:

1 client, all threads pinned, 1us trade rate:
```bash
Server:
01:36:13.608573 [I] Orders: [opn|ttl] 2,066,553|9,492,642 | Rps: 136,410
Client:
01:36:12.287540 [I] Rtt: [<50us|>50us] 99.76% avg:17us 0.24% avg:66us
```

3 clients, no client pinning, 6us trade rate:
```bash
Server:
23:10:23.747329 [I] Orders: [opn|ttl] 20,439,753|94,833,660 | Rps: 213,990
Client:
23:10:23.241168 [I] Rtt: [<50us|>50us] 98.35% avg:23us 1.65% avg:72us
23:10:23.670371 [I] Rtt: [<50us|>50us] 98.19% avg:25us 1.81% avg:76us
23:10:23.814607 [I] Rtt: [<50us|>50us] 98.53% avg:23us 1.47% avg:76us
```
