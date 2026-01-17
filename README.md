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
2026-01-17T03:25:23+01:00
Running ./run_benchmarks
Run on (16 X 5271 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 98304 KiB (x1)
Load Average: 1.24, 1.53, 1.24
***WARNING*** ASLR is enabled, the results may have unreproducible noise in them.
------------------------------------------------------------------------------
Benchmark                                    Time             CPU   Iterations
------------------------------------------------------------------------------
BM_Sys_ServerFix/ServerThroughput/1       29.8 ns         29.8 ns     23592960 1 worker(s)
BM_Sys_ServerFix/ServerThroughput/2       17.2 ns         17.2 ns     41025536 2 worker(s)
BM_Sys_ServerFix/ServerThroughput/3       14.7 ns         14.6 ns     47448064 3 worker(s)
BM_Sys_ServerFix/ServerThroughput/4       13.7 ns         13.7 ns     51249152 4 worker(s)
BM_Sys_ServerFix/ServerLatency/1           125 ns          125 ns      5556860 1 worker(s)
BM_Sys_ServerFix/ServerLatency/2           125 ns          125 ns      5591859 2 worker(s)
BM_Sys_ServerFix/ServerLatency/3           125 ns          125 ns      5590582 3 worker(s)
BM_Sys_ServerFix/ServerLatency/4           125 ns          125 ns      5531174 4 worker(s)
BM_Sys_OrderBookFix/AddOrder              18.0 ns         18.0 ns     39223296
```

Manual shared memory tests:
```bash
01:36:57.832642 [I] Rps: 5,335,230 Rtt: [<1µs|>1µs] 99.66% avg:320ns | 0.34% avg:3µs | Max:18µs
```
