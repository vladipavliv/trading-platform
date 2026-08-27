## Introduction
Ultra-low latency trading system prototype. Learning project exploring how modern exchanges work under the hood.

## Overview
The goal was to build an end-to-end trading stack - from lock-free matching core to network IPC - while staying as close to the metal as possible. The system uses wait-free SPSC queues between threads, bitmap-based price discovery, and huge pages for all hot-path data structures.

### Architecture Highlights
- **Wait-free inter-thread communication** - SPSC ring buffers with atomic operations only
- **Per-ticker sharding** - each worker thread owns its order book
- **Huge pages & cache-line alignment** - all hot-path arrays live in 2MB pages
- **CPU pinning & real-time priorities** - worker threads are pinned to dedicated cores

### Core Performance
- **Order matching**: ~18ns per order (bitmap + huge pages)
- **IPC round-trip (SHM)**: 360ns avg / <1μs @ 99th percentile
- **IPC throughput**: 7.37M messages/second
- **Network round-trip (Boost.Asio)**: 7μs avg / <10μs @ 95th percentile

## Tech Stack

### Core (hot path, zero external dependencies)
- **C++23** - `std::jthread`, `std::format`, concepts
- **Lock-free primitives** - custom SPSC/MPMC queues, atomic state machines
- **Memory** - huge pages (hugetlb), cache-line alignment, pool allocators with generation IDs

### IPC & Serialization
- **Shared Memory** - primary colocated IPC (futex-based synchronization)
- **Boost.Asio** - TCP/UDP fallback for remote clients
- **SBE** - zero-copy, fixed-size binary encoding (production path)
- **FlatBuffers** - schematized messages for tests and Python tooling

### Infrastructure (cold path, async only)
- **PostgreSQL** - reference data (tickers, users) loaded at startup
- **Apache Kafka** - post-trade telemetry and audit streaming (non-blocking)
- **Folly** - auxiliary utilities (string formatting, containers)
- **spdlog** - structured logging with compile-time level filtering

### Dev & Testing
- **CMake** - modular build with LTO (`-flto=8`)
- **Google Benchmark** - micro-benchmarks for matching engine components
- **Google Test** - unit and integration tests

## Performance

### Test Environment 
OS: Linux 7.0.0-30-generic  
CPU: AMD Ryzen 7 9800X3D 8-Core Processor (16 threads)  
RAM: No Module Installed None @ 5200 MT/s  

### Micro-benchmarks (Google Benchmark)
Benchmarks:
```bash
2026-01-17T20:39:28+01:00
Running ./run_benchmarks
Run on (16 X 5271 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 98304 KiB (x1)
Load Average: 2.13, 1.00, 0.54
***WARNING*** ASLR is enabled, the results may have unreproducible noise in them.
----------------------------------------------------------------------------
Benchmark                                  Time             CPU   Iterations
----------------------------------------------------------------------------
BM_CoordinatorFix/Throughput/1       29.1 ns         29.1 ns     24117248 1 worker(s)
BM_CoordinatorFix/Throughput/2       17.0 ns         17.0 ns     41025536 2 worker(s)
BM_CoordinatorFix/Throughput/3       14.2 ns         14.2 ns     49283072 3 worker(s)
BM_CoordinatorFix/Throughput/4       13.7 ns         13.7 ns     51380224 4 worker(s)
BM_CoordinatorFix/Latency/1           128 ns          128 ns      5505496 1 worker(s)
BM_CoordinatorFix/Latency/2           126 ns          126 ns      5570699 2 worker(s)
BM_CoordinatorFix/Latency/3           125 ns          125 ns      5417808 3 worker(s)
BM_CoordinatorFix/Latency/4           122 ns          122 ns      5719689 4 worker(s)
BM_OrderBookFix/AddOrder             17.6 ns         17.6 ns     39141376
```

### End-to-End Round-Trip Tests
Test conditions: 16M open orders limit, 8K price levels, 100 tickers.

**Shared Memory (colocated IPC):**
```bash
21:15:29.585650 [I] Rps: 7,371,150 Rtt: [<1µs|>1µs] 99.18% avg:341ns | 0.82% avg:7µs | Max:32µs
```
**Boost.Asio TCP (loopback):**
```bash
04:35:06.070989 [I] Rps: 441,436 Rtt: [<10µs|<100µs|>100µs] 95.95% avg:7µs | 4.05% avg:10µs | 0% | Max:44µs
```
