# Trading platform

## Table of contents
- [Introduction](#introduction)
    - [Server](#server)
    - [Trader](#trader)
- [Logging](#logging)
- [Configuration](#configuration)
- [Installation](#installation)
- [Usage](#usage)
    - [Run server](#run-server)
    - [Run trader](#run-trader)
- [Performance](#performance)
- [Roadmap](#potential-future-roadmap)

## Introduction
C++ client-server trading platform. Communication is done via separate TCP connections for placing orders and order status notifications, and a UDP connection for price updates.

### Server
Runs an io_context with multiple threads for network operations, a number of worker threads with a dedicated io_context for orders processing, and a system io_context for handling system events and commands. Tickers are distributed among workers and order matching is done synchronously.

### Trader
Runs an io_context with multiple threads for network operations, and a system io_context. Simplified to send random order at a given rate, receive the status, track rtt, and log the price updates.

### Logging
Logging is done via spdlog. Main logs are written to `server_log.x.txt` and `trader_log.x.txt`, status logs are written to the console.

### Configuration
Client and server use separate config files to specify url, ports, core IDs for network, application, and system threads, log level and output file, and some other settings.

## Installation

### Install required packages
```bash
sudo apt-get install -y build-essential cmake libboost-all-dev libssl-dev libpqxx-dev
```
### Install flatbuffers
```bash
git clone https://github.com/google/flatbuffers.git
cd flatbuffers
cmake -G "Unix Makefiles"
make
sudo make install
cd ..
```
### Install spdlog
```bash
git clone https://github.com/gabime/spdlog.git
cd spdlog
mkdir build
cd build
cmake ..
make
sudo make install
cd ..
```
### Setup repository
```bash
git clone https://github.com/vladipavliv/trading-platform.git
cd trading-platform
./scripts/build.sh
```

## Usage
Adjust the settings in build/server_config.ini and build/trader_config.ini

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
Trader0:
```bash
01:04:53.380633 [I] Rtt: [<50us|>50us] 99.97% avg:7us 0.03% avg:83us
```
Trader1:
```bash
01:04:49.235910 [I] Rtt: [<50us|>50us] 99.90% avg:9us 0.10% avg:123us
```
Trader2:
```bash
01:04:50.605096 [I] Rtt: [<50us|>50us] 99.85% avg:10us 0.15% avg:144us
```
Trader3:
```bash
01:04:52.118858 [I] Rtt: [<50us|>50us] 99.96% avg:7us 0.04% avg:112us
```