# Trading platform

## Table of contents
- [Introduction](#introduction)
- [Installation](#installation)
- [Usage](#usage)
    - [Setup database](#setup-database)
    - [Run server](#run-server)
    - [Run trader](#run-trader)
- [Performance](#performance)
- [Roadmap](#roadmap)

## Introduction
C++ hft platform based on boost.asio.

### Server
Runs a separate io_context on a number of threads for network operations, separate single-threaded io_context per worker for order processing, and separate single-threaded io_context for system tasks. Tickers are distributed among workers for parallel processing.

### Trader
Runs a network io_context on a number of threads, separate single-threaded io_context for orders generating and sending, and a system io_context. Worker, at the current stage, generates random orders at a given rate, tracks order status notifications, logs rtt and receives price updates.

### Logging
Logging is done with macros for compile-time exclusion and easy implementation swap. Current implementation uses spdlog. Main logs are written to a file, system logs are written to the console.

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
python3 ./scripts/create_trader.sh trader0 password0
```

### Configuration
Adjust the network, cores, trader credentials and other settings in 

`build/server/server_config.ini` and `build/trader/trader_config.ini`.

### Run server
```bash
./scripts/run.sh s
```
Type `p+`/`p-` to start/stop broadcasting price updates, `q` to shutdown.

### Run trader
```bash
./scripts/run.sh t
```
Type `t+`/`t-` to start/stop trading, `ts+`/`ts-` to +/- trading speed, `q` to shutdown.

## Performance
Tested on localhost with 4 traders and 1us trade rate

Server:
```bash
01:04:49.118586 [I] Orders: [opn|ttl] 10,813,666|50,054,412 | Rps: 374,655
```
Trader [0-4]:
```bash
01:04:53.380633 [I] Rtt: [<50us|>50us] 99.97% avg:7us 0.03% avg:83us
01:04:49.235910 [I] Rtt: [<50us|>50us] 99.90% avg:9us 0.10% avg:123us
01:04:50.605096 [I] Rtt: [<50us|>50us] 99.85% avg:10us 0.15% avg:144us
01:04:52.118858 [I] Rtt: [<50us|>50us] 99.96% avg:7us 0.04% avg:112us
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
Profile cache misses, try SBE serialization, figure batch processing threshold
