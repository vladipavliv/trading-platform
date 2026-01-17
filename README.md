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
2026-01-17T06:26:35+01:00
Running ./run_benchmarks
Run on (16 X 5228.75 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 98304 KiB (x1)
Load Average: 2.76, 1.42, 0.87
***WARNING*** ASLR is enabled, the results may have unreproducible noise in them.
------------------------------------------------------------------------------
Benchmark                                    Time             CPU   Iterations
------------------------------------------------------------------------------
BM_Sys_ServerFix/ServerThroughput/1       29.0 ns         29.0 ns     24248320 1 worker(s)
BM_Sys_ServerFix/ServerThroughput/2       17.2 ns         17.2 ns     40894464 2 worker(s)
BM_Sys_ServerFix/ServerThroughput/3       14.3 ns         14.3 ns     49152000 3 worker(s)
BM_Sys_ServerFix/ServerThroughput/4       13.6 ns         13.6 ns     51642368 4 worker(s)
BM_Sys_ServerFix/ServerLatency/1           125 ns          125 ns      5492186 1 worker(s)
BM_Sys_ServerFix/ServerLatency/2           123 ns          123 ns      5621389 2 worker(s)
BM_Sys_ServerFix/ServerLatency/3           126 ns          126 ns      5373182 3 worker(s)
BM_Sys_ServerFix/ServerLatency/4           123 ns          123 ns      5627533 4 worker(s)
BM_Sys_OrderBookFix/AddOrder              16.4 ns         16.4 ns     43139072
```

Manual shared memory tests:
```bash
01:36:57.832642 [I] Rps: 5,335,230 Rtt: [<1µs|>1µs] 99.66% avg:320ns | 0.34% avg:3µs | Max:18µs
```
Manual boost sockets tests:
```bash
04:35:06.070989 [I] Rps: 441,436 Rtt: [<10µs|<100µs|>100µs] 95.95% avg:7µs | 4.05% avg:10µs | 0% | Max:44µs
```
