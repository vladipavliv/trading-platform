# Ultra-Low Latency Trading System

Ultra-low latency trading system prototype built in C++23. A learning project focused on exchange-style architecture, lock-free concurrency, shared-memory IPC, and low-latency order matching.

## Architecture

- **Wait-free hot path** - bounded execution with atomic-only synchronization
- **Per-ticker sharding** - each worker thread owns its order book
- **Bitmap price discovery** - fast best bid/ask lookup
- **Huge pages & cache-line alignment** - optimized hot-path memory layout
- **CPU pinning & real-time priorities** - worker threads are pinned to dedicated cores
- **Shared memory IPC** - primary colocated transport
- **Boost.Asio** - TCP/UDP transport for remote clients

## Performance

### Test Environment 
OS: Ubuntu 24.04.3 LTS  
CPU: AMD Ryzen 7 9800X3D 8-Core Processor  
RAM: 64 GB @ 5200 MT/s  

### Micro-benchmarks (Google Benchmark)
```text
---------------------------------------------------------------------------
Benchmark                                 Time             CPU   Iterations
---------------------------------------------------------------------------
BM_OrderBookFix/Latency                23.3 ns         23.2 ns     29899149
BM_OrderBookFix/Throughput             20.6 ns         20.6 ns     33882112
BM_OrderBookFix/LatencyCancel          29.7 ns         29.7 ns     23704671
BM_OrderBookFix/ThroughputCancel       25.2 ns         25.2 ns     27869184
BM_CoordinatorFix/Throughput/1         40.6 ns         40.5 ns     17301504 1 worker(s)
BM_CoordinatorFix/Throughput/2         22.7 ns         22.7 ns     30801920 2 worker(s)
BM_CoordinatorFix/Throughput/3         19.9 ns         19.9 ns     35651584 3 worker(s)
BM_CoordinatorFix/Throughput/4         19.2 ns         19.2 ns     36569088 4 worker(s)
BM_CoordinatorFix/Latency/1             143 ns          143 ns      4621049 1 worker(s)
BM_CoordinatorFix/Latency/2             147 ns          147 ns      4824078 2 worker(s)
BM_CoordinatorFix/Latency/3             133 ns          133 ns      5261641 3 worker(s)
BM_CoordinatorFix/Latency/4             136 ns          136 ns      5109264 4 worker(s)
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