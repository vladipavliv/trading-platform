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
BM_OrderBookFix/Latency              22.8 ns         22.7 ns     31356997 acc=8.27k cancl=0 full=5.332k part=803 rej=0 total=14.405k
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