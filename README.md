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
With 1us delay between orders sent from trader performance is the following

Server:
```bash
14:22:57.819075 [I] Orders: [opn|ttl] 1,428,923|8,294,443 | Rps: 147,117
```
Trader:
```bash
14:22:57.722984 [I] Rtt: [<50us|>50us] 99.89% avg:17us 0.11% avg:89us | Rps: 147,001
```
