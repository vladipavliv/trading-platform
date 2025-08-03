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
Localhost, single client, all threads pinned to cores, 1us trade rate:

```bash
Server:
01:36:13.608573 [I] Orders: [opn|ttl] 2,066,553|9,492,642 | Rps: 136,410
Client:
01:36:12.287540 [I] Rtt: [<50us|>50us] 99.76% avg:17us 0.24% avg:66us
```

Localhost, 3 clients, client threads not pinned, 6us trade rate:

```bash
Server:
23:10:23.747329 [I] Orders: [opn|ttl] 20,439,753|94,833,660 | Rps: 213,990
Client:
23:10:23.241168 [I] Rtt: [<50us|>50us] 98.35% avg:23us 1.65% avg:72us
23:10:23.670371 [I] Rtt: [<50us|>50us] 98.19% avg:25us 1.81% avg:76us
23:10:23.814607 [I] Rtt: [<50us|>50us] 98.53% avg:23us 1.47% avg:76us
```
Some benchmarks:
```bash
--------------------------------------------------------------------
Benchmark                          Time             CPU   Iterations
--------------------------------------------------------------------
ServerFixture/ProcessOrders      320 ns          312 ns      2235988 <- (1 worker)
BM_protoSerialize                125 ns          124 ns      5672217
BM_protoDeserialize             93.0 ns         91.6 ns      7591133
OrderBookFixture/AddOrder       85.5 ns         84.3 ns      8224945
BM_fbsSerialize                 40.8 ns         40.2 ns     17380702
BM_fbsDeserialize               23.3 ns         23.0 ns     30676374
BM_messageBusPost               1.40 ns         1.38 ns    488956430
BM_systemBusPost                54.2 ns         53.3 ns     10000000
...
ServerFixture/RunServer          794 ns          756 ns       861042 <- (2 workers)
```
