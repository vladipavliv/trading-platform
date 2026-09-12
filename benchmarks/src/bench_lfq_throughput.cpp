/**
 * @author Vladimir Pavliv
 * @date 2026-10-09
 */

#include <atomic>
#include <thread>

#include <benchmark/benchmark.h>

#include <boost/lockfree/spsc_queue.hpp>

#include "config/server_config.hpp"
#include "containers/sequenced_spsc.hpp"
#include "containers/turbo_spsc.hpp"
#include "primitive_types.hpp"
#include "utils/spin_wait.hpp"
#include "utils/test_utils.hpp"
#include "utils/thread_utils.hpp"
#include "utils/time_utils.hpp"

namespace hft::benchmarks {
using namespace utils;
using namespace tests;
namespace {

server::ServerConfig &getConfig() {
  static server::ServerConfig cfg = [] {
    server::ServerConfig c{"bench_server_config.ini"};
    c.nsPerCycle = utils::getNsPerCycle();
    LOG_INIT(c.data);
    return c;
  }();
  return cfg;
}

constexpr size_t kQueueCapacity = 1ULL << 12;
constexpr size_t kMaxBatch = 64;

using T = uint64_t;

template <typename Q>
struct BatchAdapter;

template <size_t N>
struct BatchAdapter<SequencedSPSC<N>> {
  static size_t pushBatch(SequencedSPSC<N> &q, const T *msgs, size_t count) {
    size_t pushed = 0;
    for (size_t i = 0; i < count; ++i) {
      if (!q.write(msgs[i]))
        break;
      ++pushed;
    }
    return pushed;
  }
  static size_t popBatch(SequencedSPSC<N> &q, T *msgs, size_t count) {
    size_t popped = 0;
    for (size_t i = 0; i < count; ++i) {
      if (q.read(msgs[i]) == 0)
        break;
      ++popped;
    }
    return popped;
  }
};

template <size_t N>
struct BatchAdapter<TurboSPSC<N>> {
  static size_t pushBatch(TurboSPSC<N> &q, const T *msgs, size_t count) {
    return q.writeBatch(msgs, count);
  }
  static size_t popBatch(TurboSPSC<N> &q, T *msgs, size_t count) {
    return q.readBatch(msgs, count);
  }
};

template <size_t N>
struct BatchAdapter<boost::lockfree::spsc_queue<T, boost::lockfree::capacity<N>>> {
  using Q = boost::lockfree::spsc_queue<T, boost::lockfree::capacity<N>>;
  static size_t pushBatch(Q &q, const T *msgs, size_t count) { return q.push(msgs, count); }
  static size_t popBatch(Q &q, T *msgs, size_t count) { return q.pop(msgs, count); }
};

template <typename Queue>
void BM_LFQ_Throughput(benchmark::State &state) {
  const size_t batchSize = static_cast<size_t>(state.range(0));

  Queue queue;
  std::atomic<bool> stop{false};
  std::atomic<bool> fault{false};

  T outBatch[kMaxBatch];
  T inBatch[kMaxBatch];
  for (size_t i = 0; i < kMaxBatch; ++i) {
    outBatch[i] = static_cast<T>(i);
  }

  std::thread consumer([&] {
    utils::setThreadRealTime();
    utils::pinThreadToCore(tests::getCore(getConfig().data, 0));
    SpinWait waiter;
    while (!stop.load(std::memory_order_relaxed)) {
      size_t n = BatchAdapter<Queue>::popBatch(queue, inBatch, batchSize);
      if (n > 0) {
        waiter.reset();
      } else {
        if (!++waiter) {
          fault.store(true, std::memory_order_relaxed);
          return;
        }
      }
    }
    while (BatchAdapter<Queue>::popBatch(queue, inBatch, batchSize) > 0) {
    }
  });

  utils::setThreadRealTime();
  utils::pinThreadToCore(tests::getCore(getConfig().data, 1));

  uint64_t totalItems = 0;

  for (auto _ : state) {
    size_t pushed = BatchAdapter<Queue>::pushBatch(queue, outBatch, batchSize);
    if (pushed == 0) {
      SpinWait sw;
      while (pushed == 0) {
        pushed = BatchAdapter<Queue>::pushBatch(queue, outBatch, batchSize);
        if (pushed > 0)
          break;
        if (!++sw) {
          fault.store(true, std::memory_order_relaxed);
          break;
        }
      }
    }
    if (fault.load(std::memory_order_relaxed))
      break;
    totalItems += pushed;
  }

  stop.store(true, std::memory_order_release);
  consumer.join();

  if (fault.load(std::memory_order_relaxed)) {
    state.SkipWithError("SpinWait limit exceeded — queue may be broken");
    return;
  }

  state.SetItemsProcessed(totalItems);
}

} // namespace

BENCHMARK_TEMPLATE(BM_LFQ_Throughput, SequencedSPSC<kQueueCapacity>)
    ->Unit(benchmark::kNanosecond)
    ->UseRealTime()
    ->Arg(1)
    ->Arg(8)
    ->Arg(16)
    ->Arg(32)
    ->Arg(64)
    ->Name("BM_LFQ_Throughput_SequencedSPSC");

BENCHMARK_TEMPLATE(BM_LFQ_Throughput, TurboSPSC<kQueueCapacity>)
    ->Unit(benchmark::kNanosecond)
    ->UseRealTime()
    ->Arg(1)
    ->Arg(8)
    ->Arg(16)
    ->Arg(32)
    ->Arg(64)
    ->Name("BM_LFQ_Throughput_TurboSPSC");

BENCHMARK_TEMPLATE(BM_LFQ_Throughput,
                   boost::lockfree::spsc_queue<T, boost::lockfree::capacity<kQueueCapacity>>)
    ->Unit(benchmark::kNanosecond)
    ->UseRealTime()
    ->Arg(1)
    ->Arg(8)
    ->Arg(16)
    ->Arg(32)
    ->Arg(64)
    ->Name("BM_LFQ_Throughput_BoostSpsc");

} // namespace hft::benchmarks
