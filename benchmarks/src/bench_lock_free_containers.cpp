/**
 * @author Vladimir Pavliv
 * @date 2025-08-08
 */

#include <benchmark/benchmark.h>
#include <boost/lockfree/queue.hpp>
#include <folly/MPMCQueue.h>

#include "inline_callable.hpp"
#include "types.hpp"
#include "vyukov_queue.hpp"

namespace hft::benchmarks {

constexpr size_t capacity{1ULL << 15};

static void BM_Op_VykovMpmcQueue(benchmark::State &state) {
  auto queue = std::make_unique<VyukovQueue<size_t, capacity>>();

  size_t value{0};
  bool write{true};
  for (auto _ : state) {
    if (write && !queue->push(++value)) {
      write = false;
    }
    if (!write || queue->empty() || !queue->pop(value)) {
      write = true;
    }
  }
  benchmark::DoNotOptimize(value);
  benchmark::DoNotOptimize(write);
}
BENCHMARK(BM_Op_VykovMpmcQueue);

static void BM_Op_FollyMpmcQueue(benchmark::State &state) {
  auto queue = std::make_unique<folly::MPMCQueue<size_t>>(capacity);

  size_t value;
  bool write{true};
  for (auto _ : state) {
    if (write && !queue->write(++value)) {
      write = false;
    }
    if (!write || queue->isEmpty() || !queue->read(value)) {
      write = true;
    }
  }
  benchmark::DoNotOptimize(value);
  benchmark::DoNotOptimize(write);
}
BENCHMARK(BM_Op_FollyMpmcQueue);

static void BM_Op_BoostMpmcQueue(benchmark::State &state) {
  auto queue = std::make_unique<boost::lockfree::queue<size_t>>(capacity);

  size_t value{0};
  bool write{true};
  for (auto _ : state) {
    if (write && !queue->push(++value)) {
      write = false;
    }
    if (!write || queue->empty() || !queue->pop(value)) {
      write = true;
    }
  }
  benchmark::DoNotOptimize(value);
  benchmark::DoNotOptimize(write);
}
BENCHMARK(BM_Op_BoostMpmcQueue);

static void BM_Op_CRefHandlerCallable(benchmark::State &state) {
  size_t value;
  CRefHandler<size_t> callable{[&](CRef<size_t> val) { value = val; }};

  size_t iteration{0};
  for (auto _ : state) {
    callable(++iteration);
  }
  benchmark::DoNotOptimize(value);
  benchmark::DoNotOptimize(iteration);
}
BENCHMARK(BM_Op_CRefHandlerCallable);

static void BM_Op_InlineCallable(benchmark::State &state) {
  size_t value;
  InlineCallable<size_t> callable{[&](CRef<size_t> val) { value = val; }};

  size_t iteration{0};
  for (auto _ : state) {
    callable(++iteration);
  }
  benchmark::DoNotOptimize(value);
  benchmark::DoNotOptimize(iteration);
}
BENCHMARK(BM_Op_InlineCallable);

} // namespace hft::benchmarks
