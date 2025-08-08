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
C++ hft platform based on boost.asio, with the main focus on performance, simplicity and scalability. Key design choices:
- Dedicated thread per order-processing worker to utilize multi-core CPUs
- Distribute tickers across workers to maximize cache locality
- Fully lock-free design, no thread ever waits for another
- Interface-free, template-based design for better inlining
- Event bus for simple, decoupled communication

## Installation

### Main dependencies
Boost, Folly, FlatBuffers, Protobuf, libpqxx, librdkafka, spdlog

### Setup repository
```bash
git clone https://github.com/vladipavliv/trading-platform.git
cd trading-platform
./build.sh [options]

Options:
  c   - Clean
  d   - Debug
  t   - Include tests
  b   - Include benchmarks
  tel - Enable telemetry via kafka
```

## Usage
### Setup environment
Install Postgres, Kafka and ClickHouse. Set them up with the scripts
```bash
python3 ./scripts/setup_postgres.sh
python3 ./scripts/setup_kafka.sh
python3 ./scripts/setup_clickhouse.sh
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
  s   - Server
  c   - Client
  m   - Monitor
  ut  - Unit tests
  it  - Integration tests
  b   - Benchmarks

Optional flag:
  k   - Start Kafka
```

### Commands
```bash
Server:
  p+  - Start broadcasting price updates
  p-  - Stop broadcasting price updates
  t+  - Start telemetry streaming
  t-  - Stop telemetry streaming
  q   - Shutdown

Client:
  s+  - Start trader
  s-  - Stop trader
  t+  - Start telemetry streaming
  t-  - Stop telemetry streaming
  q   - Shutdown

Monitor:
  Supports all client and server commands
```

## Performance

Benchmarks:
```bash
Run on (20 X 4049.17 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x10)
  L1 Instruction 32 KiB (x10)
  L2 Unified 1280 KiB (x10)
  L3 Unified 24576 KiB (x1)
Load Average: 0.75, 0.92, 0.78
-------------------------------------------------------------------------
Benchmark                               Time             CPU   Iterations
-------------------------------------------------------------------------
BM_Sys_ServerFix/ProcessOrders        385 ns          383 ns      1833774 <- 1 worker
BM_Ser_ProtoSerialize                 131 ns          131 ns      5288622
BM_Ser_ProtoDeserialize              96.6 ns         96.5 ns      7198098
BM_Sys_OrderBookFix/AddOrder         89.6 ns         89.6 ns      8366584
BM_Op_MpscQueue                      12.4 ns         12.4 ns     54438828
BM_Op_FollyMpmcQueue                 53.2 ns         53.2 ns     13240825
BM_Ser_FbsSerialize                  44.1 ns         44.0 ns     16769488
BM_Ser_FbsDeserialize                23.0 ns         23.0 ns     30346047
BM_Op_MessageBusPost                 1.60 ns         1.60 ns    421585460
BM_Op_SystemBusPost                  51.3 ns         51.3 ns     10000000
...
BM_Sys_ServerFix/ProcessOrders        945 ns          920 ns       719780 <- 2 workers
BM_Sys_ServerFix/ProcessOrders        907 ns          890 ns       758570 <- 3 workers
BM_Sys_ServerFix/ProcessOrders       1036 ns         1005 ns       693849 <- 4 workers
```

Stress test (Server + Python tester with 5M pregenerated orders):
```bash
[Client 0] Loaded 5000000 orders in 0.53s
00:36:26.746280 [I] Orders: [opn|ttl] 175,658|801,343 | Rps: 801,342
00:36:27.746468 [I] Orders: [opn|ttl] 581,200|2,662,556 | Rps: 1,861,213
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
