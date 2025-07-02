# Trading platform

## Table of contents
- [Introduction](#introduction)
- [Installation](#installation)
- [Usage](#usage)
    - [Setup database](#setup-database)
    - [Run server](#run-server)
    - [Run client](#run-client)
- [Testing](#testing)    
- [Performance](#performance)
- [Roadmap](#roadmap)

## Introduction
C++ hft platform based on boost.asio, with the main focus on performance, simplicity and configurability. Key design choices:
- Use templates instead of polymorphism to maximize cache locality and inlining
- Fully lock-free design, no thread ever waits for another
- Event bus for simple, decoupled communication
- Efficient serialization with flatbuffers

### Server
Runs a separate network io_context, a number of workers for order processing with a separate io_context each, and a separate io_context for system tasks. Number of network threads and workers as well as the cores to pin them are defined in the configuration file. Tickers are distributed among workers for parallel processing.

### Client
Runs a separate network io_context, a single worker with a separate io_context for generating/sending orders, and a system io_context. Currently worker is simplified to generate random order at a given rate, track order status notifications, log rtt and receive price updates.

### Configuration
Client and server use separate config files to setup url, ports, core ids for network, application, and system threads, log file name and other settings.

## Installation

### Main dependencies
- **Boost**
- **Folly**
- **spdlog**
- **Flatbuffers**
- **Protobuf**
- **libpqxx**
- **librdkafka**

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
Adjust the network, cores, credentials and other settings in 

`build/server/server_config.ini` and `build/client/client_config.ini`.

### Run server
```bash
./run.sh s
```
Type `p+`/`p-` to start/stop broadcasting price updates, `q` to shutdown.

### Run client
```bash
./run.sh c
```
Type `t+`/`t-` to start/stop trading, `ts+`/`ts-` to +/- trading speed, `k+`/`k-` to start/stop kafka metrics streaming, `q` to shutdown.

## Testing
Testing strategy is still in consideration. As interfaces have not been used for maximum blazing fastness, and only some components are templated, mocking possibilities are quite limited.

The current ideas:
- Communication is highly decoupled via the bus, so alot of stuff could be tested without mocks
- Not every component needs to be mocked, hot paths could be tested as is
- Performance-critical components, like the network layer, could be templated and mocked
- Less performance-critical components, like adapters, could use interfaces for higher configurability

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

## Roadmap
- [x] **Stream metrics to kafka**  
Offload the latencies and other metadata to kafka
- [ ] **Monitor the metrics**  
Make separate monitoring service, maybe using Go
- [ ] **Persist the data**  
Persist metadata to ClickHouse and maybe currently opened orders to Postgres on a shutdown
- [ ] **Improve authentication**  
Encrypt the password, use nonce
- [ ] **Ticker rerouting**  
Dynamically rerout the tickers to a different worker, add/remove workers, load balancing
- [ ] **Proper trading strategy**  
And price fluctuations
- [ ] **Optimize**  
Profile cache misses, try SBE serialization
- [ ] **Improve metrics streaming**  
Make separate DataBus to offload intense metadata streaming traffic from SystemBus
- [ ] **Contribute to ClickHouse**  
Investigate the possibility of adding FlatBuffers support to ClickHouse for direct consumption from kafka
