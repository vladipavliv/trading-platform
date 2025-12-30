/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include <benchmark/benchmark.h>

#include "bus/bus_holder.hpp"
#include "domain_types.hpp"
#include "lfq_runner.hpp"
#include "types.hpp"
#include "utils/test_data.hpp"
#include "utils/utils.hpp"

namespace hft::benchmarks {

using namespace utils;
using namespace server;

constexpr size_t CAPACITY = 65536;
constexpr size_t TICKER_COUNT = 10;
constexpr size_t ORDER_COUNT = 16384 * 128;

static void DISABLED_BM_Op_MessageBusPost(benchmark::State &state) {
  size_t counter{0};
  const auto order = generateOrder();

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

  pinThreadToCore(4);

  SystemBus bus;
  Thread t{[&bus]() { bus.run(); }};
  bus.subscribe<Order>([](CRef<Order> o) { o.partialFill(1); });

  for (auto _ : state) {
    bus.post(order);
    benchmark::DoNotOptimize(&order);
  }
  bus.stop();
}
BENCHMARK(DISABLED_BM_Op_SystemBusPost);

static void BM_Op_LfqRunnerThroughput(benchmark::State &state) {
  TestTickerData tkrData{TICKER_COUNT};
  TestOrderData orData{tkrData, ORDER_COUNT};

  pinThreadToCore(2);

  Consumer consumer;

  SystemBus bus;
  LfqRunner<ServerOrder, Consumer, CAPACITY> lfqRunner(4, consumer, ErrorBus{bus});
  lfqRunner.run();

  while (state.KeepRunningBatch(ORDER_COUNT)) {
    consumer.processed = 0;
    consumer.signal.clear();

    for (auto &order : orData.orders) {
      auto tid = (uint32_t)order.order.ticker[0];
      benchmark::DoNotOptimize(tid);

      if (!lfqRunner.post(order)) {
        asm volatile("pause" ::: "memory");
      }
    }

    while (!consumer.signal.test(std::memory_order_acquire)) {
      asm volatile("pause" ::: "memory");
    }
    benchmark::DoNotOptimize(consumer.processed);
    benchmark::DoNotOptimize(consumer.signal);
  }

  lfqRunner.stop();
}
BENCHMARK(BM_Op_LfqRunnerThroughput);

static void BM_Op_StreamBusThroughput(benchmark::State &state) {
  TestTickerData tkrData{TICKER_COUNT};
  TestOrderData orData{tkrData, ORDER_COUNT};

  pinThreadToCore(2);

  alignas(64) std::atomic<uint64_t> processed;
  alignas(64) std::atomic_flag signal;

  SystemBus systemBus;
  StreamBus<CAPACITY, ServerOrder> streamBus{Config::get<CoreId>("bench.stream_core"), systemBus};

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

} // namespace hft::benchmarks
