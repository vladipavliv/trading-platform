/**
 * @author Vladimir Pavliv
 * @date 2025-04-07
 */

#include <benchmark/benchmark.h>

#include <folly/MPMCQueue.h>

#include "mpsq_queue.hpp"
#include "types.hpp"

namespace hft::benchmarks {

static void BM_Op_MpscQueue(benchmark::State &state) {
  const size_t capacity{1024 * 64};
  auto lfq = std::make_unique<MpscQueue<String, capacity>>();

  String readVal;
  bool write{true};
  for (auto _ : state) {
    if (write && !lfq->enqueue(String{"value"})) {
      write = false;
    }
    if (!write || lfq->empty() || !lfq->dequeue(readVal)) {
      write = true;
    }
  }
  benchmark::DoNotOptimize(readVal);
  benchmark::DoNotOptimize(write);
}
BENCHMARK(BM_Op_MpscQueue);

static void BM_Op_FollyMpmcQueue(benchmark::State &state) {
  constexpr size_t capacity{1024 * 64};
  auto lfq = std::make_unique<folly::MPMCQueue<String>>(capacity);

  String readVal;
  bool write{true};
  for (auto _ : state) {
    if (write && !lfq->write(String{"value"})) {
      write = false;
    }
    if (!write || lfq->isEmpty() || !lfq->read(readVal)) {
      write = true;
    }
  }
  benchmark::DoNotOptimize(readVal);
  benchmark::DoNotOptimize(write);
}
BENCHMARK(BM_Op_FollyMpmcQueue);

} // namespace hft::benchmarks
