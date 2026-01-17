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
2026-01-13T13:22:35+01:00
Running ./run_benchmarks
Run on (16 X 5271 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 98304 KiB (x1)
Load Average: 0.93, 0.52, 0.54
***WARNING*** ASLR is enabled, the results may have unreproducible noise in them.
------------------------------------------------------------------------
Benchmark                              Time             CPU   Iterations
------------------------------------------------------------------------
BM_Sys_ServerFix/Throughput/1       62.8 ns         62.8 ns     11272192 1 worker(s)
BM_Sys_ServerFix/Throughput/2       37.5 ns         37.5 ns     18874368 2 worker(s)
BM_Sys_ServerFix/Throughput/3       27.4 ns         27.4 ns     25690112 3 worker(s)
BM_Sys_ServerFix/Throughput/4       22.1 ns         22.1 ns     31719424 4 worker(s)
BM_Sys_ServerFix/Latency/1           151 ns          151 ns      4408122 1 worker(s)
BM_Sys_ServerFix/Latency/2           163 ns          163 ns      4190611 2 worker(s)
BM_Sys_ServerFix/Latency/3           150 ns          150 ns      4552097 3 worker(s)
BM_Sys_ServerFix/Latency/4           145 ns          145 ns      4755725 4 worker(s)
BM_Sys_OrderBookFix/AddOrder        42.0 ns         42.0 ns     16433152
BM_Op_LfqRunnerThroughput           19.9 ns         19.9 ns     35651584
BM_Op_LfqRunnerTailSpy              17.7 ns         17.7 ns     39845888 Max_ns=912.885 Min_ns=0 P50_ns=9.03846 P99.9_ns=18.0769 P99_ns=18.0769
```

Manual shared memory tests:
```bash
01:36:57.832642 [I] Rps: 5,335,230 Rtt: [<1µs|>1µs] 99.66% avg:320ns | 0.34% avg:3µs | Max:18µs
```
