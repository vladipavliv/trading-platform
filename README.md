# Trading platform simulator
---

## Introduction
Simple Client-Server trading platform. Allows you to place an order, receive order status and price updates. Communication is done via separate Tcp connections to send orders and receive status, and Udp connection to broadcast price updates.

### Server
Uses separate io_context for network io operations, and N additional threads to process orders. Each worker thread runs a separate io_context, tickers are divided between threads and orders are processed synchronously.

### Trader
Simplified to send random orders at a given rate, receive statuses, track rtt, and log price updates.

### Logging
Main logs are written to build/server_log.x.txt and build/trader_log.x.txt. Status logs are written to the console
---

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
./build.sh
```
---

## Usage
Adjust the settings in build/server_config.ini and build/trader_config.ini

### Server
Run the server: 
```bash
./run.sh s
```
Type p+/p- to start/stop broadcasting price updates, q to shutdown.

### Trader
Run the trader: 
```bash
./run.sh t
```
Type t+/t- to start/stop trading, ts+/ts- to +/- trading speed, q to shutdown.

## Performance
Tested on localhost without any core isolation or other tweaking
Last/Best performance check for 5us trade rate and 4 server cores:<br>
Server:<br>
```bash
22:25:08.702500 [I] Orders [matched|total] 2307286 2961784 rps:101929<br>
```
Trader:<br>
```bash
22:25:08.677493 [I] RTT [1us|100us|1ms]  100.00% avg:18us  0.00% avg:135us  0.00% avg:0ms<br>
```