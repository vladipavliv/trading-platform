/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include <benchmark/benchmark.h>

#include "bus/bus_hub.hpp"
#include "config/config.hpp"
#include "domain_types.hpp"
#include "primitive_types.hpp"
#include "utils/data_generator.hpp"
#include "utils/lfq_runner.hpp"
#include "utils/post_tracker.hpp"
#include "utils/test_utils.hpp"

namespace hft::benchmarks {

using namespace utils;
using namespace server;
using namespace tests;

constexpr size_t TICKER_COUNT = 10;
constexpr size_t ORDER_COUNT = 16384 * 128;
constexpr double CPU_FREQ = 5.2;

static void DISABLED_BM_LfqRunnerThroughput(benchmark::State &state) {
  using Runner = LfqRunner<InternalOrderEvent, PostTracker, SystemBus>;

  GenTickerData tkrData{TICKER_COUNT};
  GenOrderData orData{tkrData, ORDER_COUNT};

  pinThreadToCore(getCore(0));

  PostTracker consumer{ORDER_COUNT - 1};

  SystemBus bus;
  auto lfqRunner = std::make_unique<Runner>(consumer, bus, getCore(1));
  lfqRunner->run();

  while (state.KeepRunningBatch(ORDER_COUNT)) {
    consumer.processed = 0;
    consumer.clear();

    for (auto &order : orData.orders) {
      lfqRunner->post(order);
    }

    while (consumer.signal.load(std::memory_order_acquire) == 0) {
      asm volatile("pause" ::: "memory");
    }
    benchmark::DoNotOptimize(consumer.processed);
    benchmark::DoNotOptimize(consumer.signal);
  }

  lfqRunner->stop();
}
BENCHMARK(DISABLED_BM_LfqRunnerThroughput);

static void DISABLED_BM_LfqRunnerTailSpy(benchmark::State &state) {
  using Runner = LfqRunner<InternalOrderEvent, PostTracker, SystemBus>;

  GenTickerData tkrData{TICKER_COUNT};
  GenOrderData orData{tkrData, ORDER_COUNT};
  pinThreadToCore(getCore(0));

  PostTracker consumer{ORDER_COUNT - 1};

  SystemBus bus;
  auto lfqRunner = std::make_unique<Runner>(consumer, bus, getCore(1));
  lfqRunner->run();

  std::vector<uint64_t> tscLogs(ORDER_COUNT + 1);

  while (state.KeepRunningBatch(ORDER_COUNT)) {
    consumer.processed = 0;
    consumer.clear();

    size_t cycle = 0;
    for (auto &order : orData.orders) {
      uint64_t start = __rdtsc();

      lfqRunner->post(order);

      tscLogs[cycle++] = __rdtsc() - start;
    }

    while (consumer.signal.load(std::memory_order_acquire) == 0) {
      asm volatile("pause" ::: "memory");
    }
  }

  std::sort(tscLogs.begin(), tscLogs.end());

  state.counters["Min_ns"] = tscLogs[0] / CPU_FREQ;
  state.counters["P50_ns"] = tscLogs[ORDER_COUNT / 2] / CPU_FREQ;
  state.counters["P99_ns"] = tscLogs[ORDER_COUNT * 99 / 100] / CPU_FREQ;
  state.counters["P99.9_ns"] = tscLogs[ORDER_COUNT * 999 / 1000] / CPU_FREQ;
  state.counters["Max_ns"] = tscLogs[ORDER_COUNT - 1] / CPU_FREQ;

  lfqRunner->stop();
}
BENCHMARK(DISABLED_BM_LfqRunnerTailSpy);

} // namespace hft::benchmarks
