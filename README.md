# Trading platform

## Table of contents
- [Introduction](#introduction)
- [Installation](#installation)
- [Usage](#usage)
    - [Setup environment](#setup-environment)
    - [Configuration](#configuration)
    - [Run the server](#run-the-server)
    - [Run the client](#run-the-client)
    - [Run the monitor](#run-monitor)
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
./build.sh
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

### Run the server
```bash
./run.sh s k
```
Type `p+`/`p-` to start/stop broadcasting price updates, `q` to shutdown.

### Run the client
```bash
./run.sh c k
```
Type `t+`/`t-` to start/stop trading, `ts+`/`ts-` to +/- trading speed, `k+`/`k-` to start/stop kafka metrics streaming, `q` to shutdown.

### Run the monitor
```bash
./run.sh m k
```
Enter client or server command, kafka should be running and enabled in configuration.

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
----------------------------------------------------------------------
Benchmark                            Time             CPU   Iterations
----------------------------------------------------------------------
ServerFixture/ProcessOrders        377 ns          374 ns      1932526 <- 1 worker
BM_protoSerialize                  127 ns          127 ns      5416681
BM_protoDeserialize               92.9 ns         92.9 ns      7218331
OrderBookFixture/AddOrder         73.4 ns         73.3 ns      8641751
BM_fbsSerialize                   44.4 ns         44.4 ns     16402887
BM_fbsDeserialize                 20.8 ns         20.7 ns     32475925
BM_messageBusPost                 1.34 ns         1.34 ns    520477008
BM_systemBusPost                  51.4 ns         51.3 ns     10000000
...
ServerFixture/ProcessOrders       1004 ns          977 ns       707598 <- 2 workers
ServerFixture/ProcessOrders        952 ns          934 ns       671744 <- 3 workers
ServerFixture/ProcessOrders        972 ns          951 ns       688096 <- 4 workers
```

Localhost tests:

1 client, all threads pinned, 1us trade rate:
```bash
Server:
01:36:13.608573 [I] Orders: [opn|ttl] 2,066,553|9,492,642 | Rps: 136,410
Client:
01:36:12.287540 [I] Rtt: [<50us|>50us] 99.76% avg:17us 0.24% avg:66us
```

3 clients, client threads not pinned, 6us trade rate:
```bash
Server:
23:10:23.747329 [I] Orders: [opn|ttl] 20,439,753|94,833,660 | Rps: 213,990
Client:
23:10:23.241168 [I] Rtt: [<50us|>50us] 98.35% avg:23us 1.65% avg:72us
23:10:23.670371 [I] Rtt: [<50us|>50us] 98.19% avg:25us 1.81% avg:76us
23:10:23.814607 [I] Rtt: [<50us|>50us] 98.53% avg:23us 1.47% avg:76us
```
