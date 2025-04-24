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
C++ hft platform based on boost.asio.

### Server
Runs a separate network io_context, a number of workers for order processing with a separate io_context each, and a separate io_context for system tasks. Number of network threads and workers as well as the cores to pin them are defined in the configuration file. Tickers are distributed among workers for parallel processing.

### Client
Runs a separate network io_context, a single worker with a separate io_context for generating/sending orders, and a system io_context. Currently worker is simplified to generate random order at a given rate, track order status notifications, log rtt and receive price updates.

### Logging
Logging is done with macros for compile-time exclusion and easy implementation swap. Currently spdlog is used. Main logs are written to a file, system logs are written to the console.

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
./scripts/build.sh
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
./run.sh t
```
Type `t+`/`t-` to start/stop trading, `ts+`/`ts-` to +/- trading speed, `k+`/`k-` to start/stop kafka metrics streaming, `q` to shutdown.

## Testing
The main goal for the project was performance, so interfaces and inheritance have not been used. Instead everything is templated.
Interfaces, however, have been implemented for tests to inject a proper mock and use gmock capabilities for advanced mocking.

## Performance
Tested on localhost with 1us trade rate

Server:
```bash
23:09:06.759313 [I] Orders: [opn|ttl] 1,339,716|6,121,796 | Rps: 138,647
```
Client:
```bash
23:09:06.926566 [I] Rtt: [<50us|>50us] 99.82% avg:17us 0.18% avg:59us
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
- [ ] **Testing**  
Add unit, integration and stress tests and run the cicd pipeline
- [ ] **Ticker rerouting**  
Dynamically rerout the tickers to a different worker, add/remove workers, load balancing
- [ ] **Proper trading strategy**  
And price fluctuations
- [ ] **Optimize**  
Profile cache misses, try SBE serialization
- [ ] **Improve metrics streaming**  
Make separate DataBus
- [ ] **Contribute to ClickHouse**  
Investigate the possibility to add flatbuffers support to ClickHouse for direct consuming from kafka
