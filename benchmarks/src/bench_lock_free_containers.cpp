/**
 * @author Vladimir Pavliv
 * @date 2025-08-08
 */

#include <thread>

#include <benchmark/benchmark.h>
#include <boost/lockfree/queue.hpp>
#include <folly/MPMCQueue.h>

#include "config/server_config.hpp"
#include "containers/batch_spsc.hpp"
#include "containers/sequenced_spsc.hpp"
#include "containers/vyukov_mpmc.hpp"
#include "primitive_types.hpp"
#include "ptr_types.hpp"
#include "utils/spin_wait.hpp"
#include "utils/string_utils.hpp"
#include "utils/test_utils.hpp"
#include "utils/thread_utils.hpp"

namespace hft::benchmarks {
using namespace server;
using namespace tests;
using namespace utils;

namespace {
constexpr size_t N{1ULL << 15};
using T = size_t;
inline const ServerConfig cfg{"bench_server_config.ini"};

template <typename Q>
struct QueueAdapter {
  static UPtr<Q> create() { return std::make_unique<Q>(); }
  static bool push(Q &q, T val) { return q.push(val); }
  static bool pop(Q &q, T &val) { return q.pop(val); }
};

template <>
struct QueueAdapter<SequencedSPSC<N>> {
  using Queue = SequencedSPSC<N>;

  static UPtr<Queue> create() { return std::make_unique<Queue>(); }
  static bool push(Queue &q, T val) { return q.write(val); }
  static bool pop(Queue &q, T &val) { return q.read(val) > 0; }
};

template <>
struct QueueAdapter<VyukovMPMC<T, N>> {
  using Queue = VyukovMPMC<T, N>;

  static UPtr<Queue> create() { return std::make_unique<Queue>(); }
  static bool push(Queue &q, T val) { return q.push(val); }
  static bool pop(Queue &q, T &val) { return q.pop(val) > 0; }
};

template <>
struct QueueAdapter<BatchSPSC<N>> {
  using Queue = BatchSPSC<N>;

  static UPtr<Queue> create() { return std::make_unique<Queue>(); }
  static bool push(Queue &q, T val) { return q.write(val); }
  static bool pop(Queue &q, T &val) { return q.read(val) > 0; }
};

template <>
struct QueueAdapter<folly::MPMCQueue<T>> {
  using Queue = folly::MPMCQueue<T>;

  static UPtr<Queue> create() { return std::make_unique<Queue>(); }
  static bool push(Queue &q, T val) { return q.write(val); }
  static bool pop(Queue &q, T &val) { return q.read(val); }
};

template <>
struct QueueAdapter<boost::lockfree::queue<T>> {
  using Queue = boost::lockfree::queue<T>;

  static UPtr<Queue> create() { return std::make_unique<Queue>(); }
  static bool push(Queue &q, T val) { return q.push(val); }
  static bool pop(Queue &q, T &val) { return q.pop(val); }
};
} // namespace

template <typename QueueType>
static void BM_GenericQueue(benchmark::State &state) {
  auto queue = QueueAdapter<QueueType>::create();

  size_t produced{0};
  std::atomic_size_t consumed = 0;

  // utils::setThreadRealTime();
  utils::pinThreadToCore(tests::getCore(cfg.data, 0));

  std::jthread consumerThread{[&](std::stop_token token) {
    // utils::setThreadRealTime();
    utils::pinThreadToCore(tests::getCore(cfg.data, 1));

    SpinWait waiter;
    while (!token.stop_requested()) {
      T value = 0;
      if (QueueAdapter<QueueType>::pop(*queue, value)) {
        waiter.reset();
        consumed.fetch_add(1, std::memory_order_release);
      } else if (!++waiter) {
        LOG_ERROR_SYSTEM("Failed to consume");
        break;
      }
    }
  }};

  for (auto _ : state) {
    SpinWait waiter;
    while (!QueueAdapter<QueueType>::push(*queue, produced)) {
      if (!++waiter) {
        LOG_ERROR_SYSTEM("Failed to produce");
        break;
      }
    }
    ++produced;
  }
  SpinWait waiter;
  while (consumed.load(std::memory_order_acquire) < produced) {
    if (!++waiter) {
      LOG_ERROR_SYSTEM("Failed to consume {}<{}", consumed.load(std::memory_order_acquire),
                       produced);
      break;
    }
  }
  consumerThread.request_stop();
  benchmark::DoNotOptimize(produced);
}

BENCHMARK_TEMPLATE(BM_GenericQueue, SequencedSPSC<N>);
BENCHMARK_TEMPLATE(BM_GenericQueue, VyukovMPMC<T, N>);
BENCHMARK_TEMPLATE(BM_GenericQueue, BatchSPSC<N>);
BENCHMARK_TEMPLATE(BM_GenericQueue, boost::lockfree::queue<T>);

} // namespace hft::benchmarks
