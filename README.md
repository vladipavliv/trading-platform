# Trading platform

## Table of contents
- [Introduction](#introduction)
- [Installation](#installation)
- [Usage](#usage)
    - [Setup database](#setup-database)
    - [Run server](#run-server)
    - [Run client](#run-client)
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

### Setup repository
```bash
git clone https://github.com/vladipavliv/trading-platform.git
cd trading-platform
./scripts/build.sh
```

## Usage
### Setup database
Create db, tables, and generate some data
```bash
python3 ./scripts/create_tables.sh
python3 ./scripts/generate_tickers.sh 1000
python3 ./scripts/create_client.sh client0 password0
```

### Configuration
Adjust the network, cores, credentials and other settings in 

`build/server/server_config.ini` and `build/client/client_config.ini`.

### Run server
```bash
./scripts/run.sh s
```
Type `p+`/`p-` to start/stop broadcasting price updates, `q` to shutdown.

### Run client
```bash
./scripts/run.sh t
```
Type `t+`/`t-` to start/stop trading, `ts+`/`ts-` to +/- trading speed, `q` to shutdown.

## Performance
Tested on localhost with 2 clients and 1us trade rate

Server:
```bash
10:54:19.758093 [I] Orders: [opn|ttl] 4,826,034|22,285,931 | Rps: 153,804
```
Client [0-1]:
```bash
10:54:21.339249 [I] Rtt: [<50us|>50us] 96.31% avg:26us 3.69% avg:64us
10:54:19.481263 [I] Rtt: [<50us|>50us] 93.74% avg:30us 6.26% avg:64us
```

## Roadmap
- [ ] **Data persist and monitoring**  
Offload all the data to kafka, persist it and make some monitoring frontend
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
