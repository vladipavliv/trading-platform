/**
 * @author Vladimir Pavliv
 * @date 2025-08-08
 */

#include <benchmark/benchmark.h>
#include <boost/lockfree/queue.hpp>
#include <folly/MPMCQueue.h>

#include "containers/vyukov_mpmc.hpp"
#include "primitive_types.hpp"

namespace hft::benchmarks {

constexpr size_t capacity{1ULL << 15};

static void DISABLED_BM_VykovMpmcQueue(benchmark::State &state) {
  auto queue = std::make_unique<VyukovMPMC<size_t, capacity>>();

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
BENCHMARK(DISABLED_BM_VykovMpmcQueue);

static void DISABLED_BM_BoostMpmcQueue(benchmark::State &state) {
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
BENCHMARK(DISABLED_BM_BoostMpmcQueue);

static void DISABLED_BM_FollyMpmcQueue(benchmark::State &state) {
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
BENCHMARK(DISABLED_BM_FollyMpmcQueue);

} // namespace hft::benchmarks
