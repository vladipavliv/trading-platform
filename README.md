## Introduction
Ultra-low latency trading ecosystem built with C++ using lock-free primitives, templates, and cache-coherent memory layouts.

## Tech stack
- Language: C++20 (Atomics, Templates, Concepts)
- Networking: Boost.Asio(TCP/UDP), Shared Memory
- Serialization: SBE (Simple Binary Encoding), FlatBuffers
- Infrastructure: Postgres, Kafka, Folly, Spdlog

## Performance

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
BM_ServerFix/InternalThroughput/1       29.1 ns         29.1 ns     24117248 1 worker(s)
BM_ServerFix/InternalThroughput/2       17.0 ns         17.0 ns     41025536 2 worker(s)
BM_ServerFix/InternalThroughput/3       14.2 ns         14.2 ns     49283072 3 worker(s)
BM_ServerFix/InternalThroughput/4       13.7 ns         13.7 ns     51380224 4 worker(s)
BM_ServerFix/InternalLatency/1           128 ns          128 ns      5505496 1 worker(s)
BM_ServerFix/InternalLatency/2           126 ns          126 ns      5570699 2 worker(s)
BM_ServerFix/InternalLatency/3           125 ns          125 ns      5417808 3 worker(s)
BM_ServerFix/InternalLatency/4           122 ns          122 ns      5719689 4 worker(s)
BM_OrderBookFix/AddOrder                17.6 ns         17.6 ns     39141376
```

Manual tests (16m opened orders limit, 100k price levels, 100 tickers)

Shared memory:
```bash
23:32:12.712988 [I] Rps: 5,457,693 Rtt: [<1µs|>1µs] 99.73% avg:338ns | 0.27% avg:3µs | Max:20µs
```
Boost sockets:
```bash
04:35:06.070989 [I] Rps: 441,436 Rtt: [<10µs|<100µs|>100µs] 95.95% avg:7µs | 4.05% avg:10µs | 0% | Max:44µs
```
