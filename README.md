## Introduction
Ultra-low latency trading ecosystem built with C++20 using lock-free primitives, templates, and cache-coherent memory layouts.

## Tech stack
- Language: C++20 (Atomics, Templates, Concepts), Python
- Networking: Boost.Asio(TCP/UDP), Shared Memory
- Serialization: SBE (Simple Binary Encoding), FlatBuffers
- Infrastructure: Postgres, Kafka, Folly, Spdlog

## Performance

Benchmarks:
```bash
2026-01-17T03:40:45+01:00
Running ./run_benchmarks
Run on (16 X 5232.94 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 98304 KiB (x1)
Load Average: 0.09, 0.25, 0.57
***WARNING*** ASLR is enabled, the results may have unreproducible noise in them.
------------------------------------------------------------------------------
Benchmark                                    Time             CPU   Iterations
------------------------------------------------------------------------------
BM_Sys_ServerFix/ServerThroughput/1       29.3 ns         29.3 ns     23461888 1 worker(s)
BM_Sys_ServerFix/ServerThroughput/2       17.2 ns         17.2 ns     40632320 2 worker(s)
BM_Sys_ServerFix/ServerThroughput/3       14.2 ns         14.2 ns     49283072 3 worker(s)
BM_Sys_ServerFix/ServerThroughput/4       13.3 ns         13.3 ns     52822016 4 worker(s)
BM_Sys_ServerFix/ServerLatency/1           118 ns          118 ns      5714018 1 worker(s)
BM_Sys_ServerFix/ServerLatency/2           121 ns          121 ns      5725767 2 worker(s)
BM_Sys_ServerFix/ServerLatency/3           122 ns          122 ns      5686773 3 worker(s)
BM_Sys_ServerFix/ServerLatency/4           123 ns          123 ns      5751560 4 worker(s)
BM_Sys_OrderBookFix/AddOrder              18.7 ns         18.7 ns     37879808
```

Manual shared memory tests:
```bash
01:36:57.832642 [I] Rps: 5,335,230 Rtt: [<1µs|>1µs] 99.66% avg:320ns | 0.34% avg:3µs | Max:18µs
```
Manual boost sockets tests:
```bash
04:35:06.070989 [I] Rps: 441,436 Rtt: [<10µs|<100µs|>100µs] 95.95% avg:7µs | 4.05% avg:10µs | 0% | Max:44µs
```
