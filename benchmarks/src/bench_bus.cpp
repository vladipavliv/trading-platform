/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include <benchmark/benchmark.h>

#include "bus/bus_holder.hpp"
#include "config/config.hpp"
#include "domain_types.hpp"
#include "lfq_runner.hpp"
#include "types.hpp"
#include "utils/bench_utils.hpp"
#include "utils/test_data.hpp"
#include "utils/utils.hpp"

namespace hft::benchmarks {

using namespace utils;
using namespace server;

constexpr size_t LFQ_CAPACITY = 65536;
constexpr size_t TICKER_COUNT = 10;
constexpr size_t ORDER_COUNT = 16384 * 128;
constexpr double CPU_FREQ = 4.6;

static void BM_Op_LfqRunnerThroughput(benchmark::State &state) {
  using Runner = LfqRunner<ServerOrder, Consumer, LFQ_CAPACITY>;

  TestTickerData tkrData{TICKER_COUNT};
  TestOrderData orData{tkrData, ORDER_COUNT};

  pinThreadToCore(getCore(0));

  Consumer consumer;

  SystemBus bus;
  auto lfqRunner = std::make_unique<Runner>(getCore(1), consumer, ErrorBus{bus});
  lfqRunner->run();

  while (state.KeepRunningBatch(ORDER_COUNT)) {
    consumer.processed = 0;
    consumer.signal.clear();

    for (auto &order : orData.orders) {
      auto tid = (uint32_t)order.order.ticker[0];
      benchmark::DoNotOptimize(tid);

      if (!lfqRunner->post(order)) {
        asm volatile("pause" ::: "memory");
      }
    }

    while (!consumer.signal.test(std::memory_order_acquire)) {
      asm volatile("pause" ::: "memory");
    }
    benchmark::DoNotOptimize(consumer.processed);
    benchmark::DoNotOptimize(consumer.signal);
  }

  lfqRunner->stop();
}
BENCHMARK(BM_Op_LfqRunnerThroughput);

static void BM_Op_LfqRunnerTailSpy(benchmark::State &state) {
  using Runner = LfqRunner<ServerOrder, Consumer, LFQ_CAPACITY>;

  TestTickerData tkrData{TICKER_COUNT};
  TestOrderData orData{tkrData, ORDER_COUNT};
  pinThreadToCore(getCore(0));

  Consumer consumer;
  SystemBus bus;
  auto lfqRunner = std::make_unique<Runner>(getCore(1), consumer, ErrorBus{bus});
  lfqRunner->run();

  std::vector<uint64_t> tscLogs(ORDER_COUNT + 1);

  while (state.KeepRunningBatch(ORDER_COUNT)) {
    consumer.processed = 0;
    consumer.signal.clear();

    size_t cycle = 0;
    for (auto &order : orData.orders) {
      uint64_t start = __rdtsc();

      while (!lfqRunner->post(order)) {
        asm volatile("pause" ::: "memory");
      }

      tscLogs[cycle++] = __rdtsc() - start;
    }

    while (!consumer.signal.test(std::memory_order_acquire)) {
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
BENCHMARK(BM_Op_LfqRunnerTailSpy);

static void BM_Op_StreamBusThroughput(benchmark::State &state) {
  TestTickerData tkrData{TICKER_COUNT};
  TestOrderData orData{tkrData, ORDER_COUNT};

  pinThreadToCore(getCore(0));

  alignas(64) std::atomic<uint64_t> processed;
  alignas(64) std::atomic_flag signal;

  SystemBus systemBus;
  StreamBus<LFQ_CAPACITY, ServerOrder> streamBus{getCore(1), systemBus};

  streamBus.subscribe<ServerOrder>([&processed, &signal](CRef<ServerOrder> o) {
    if (processed.fetch_add(1, std::memory_order_relaxed) + 1 == ORDER_COUNT) {
      signal.test_and_set(std::memory_order_release);
    }
  });
  streamBus.run();

  while (state.KeepRunningBatch(ORDER_COUNT)) {
    processed = 0;
    signal.clear();

    for (auto &order : orData.orders) {
      auto tid = (uint32_t)order.order.ticker[0];
      benchmark::DoNotOptimize(tid);

      while (!streamBus.post(order)) {
        asm volatile("pause" ::: "memory");
      }
    }

    while (!signal.test(std::memory_order_acquire)) {
      asm volatile("pause" ::: "memory");
    }
    benchmark::DoNotOptimize(processed);
    benchmark::DoNotOptimize(signal);
  }

  streamBus.stop();
}
BENCHMARK(BM_Op_StreamBusThroughput);

static void DISABLED_BM_Op_MessageBusPost(benchmark::State &state) {
  size_t counter{0};
  const auto order = generateOrder();

  pinThreadToCore(getCore(0));

  MessageBus<Order> bus;
  bus.subscribe<Order>([&counter, &state, order](CRef<Order> o) { counter += o.id; });

  for (auto _ : state) {
    bus.post<Order>(order);
    benchmark::DoNotOptimize(&order);
    benchmark::DoNotOptimize(counter);
  }
}
BENCHMARK(DISABLED_BM_Op_MessageBusPost);

static void DISABLED_BM_Op_SystemBusPost(benchmark::State &state) {
  const auto order = generateOrder();

  pinThreadToCore(getCore(0));

  SystemBus bus;
  Thread t{[&bus]() {
    pinThreadToCore(getCore(1));
    bus.run();
  }};
  bus.subscribe<Order>([](CRef<Order> o) { o.partialFill(1); });

  for (auto _ : state) {
    bus.post(order);
    benchmark::DoNotOptimize(&order);
  }
  bus.stop();
}
BENCHMARK(DISABLED_BM_Op_SystemBusPost);

} // namespace hft::benchmarks
