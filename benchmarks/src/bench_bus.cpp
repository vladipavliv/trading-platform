/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include <benchmark/benchmark.h>

#include "bus/bus_holder.hpp"
#include "domain_types.hpp"
#include "lfq_runner.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft::benchmarks {

static void DISABLED_BM_Op_MessageBusPost(benchmark::State &state) {
  size_t counter{0};
  const auto order = utils::generateOrder();

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
  const auto order = utils::generateOrder();

  utils::pinThreadToCore(4);

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

struct alignas(64) Consumer {
  alignas(64) std::atomic<uint64_t> consumed;

  template <typename Message>
  void post(const Message &msg) {
    consumed.fetch_add(1, std::memory_order_relaxed);
  }
};

static void BM_Op_LfqRunner(benchmark::State &state) {
  const auto order = utils::generateOrder();

  utils::pinThreadToCore(2);

  uint64_t produced = 0;
  Consumer consumer;
  SystemBus bus;

  LfqRunner<Order, Consumer> lfqRunner(4, consumer, ErrorBus{bus});
  lfqRunner.run();

  for (auto _ : state) {
    auto tid = (uint32_t)order.ticker[0];
    benchmark::DoNotOptimize(tid);

    if (!lfqRunner.post(order)) {
      asm volatile("pause" ::: "memory");
    }
    ++produced;
    benchmark::DoNotOptimize(produced);
  }

  size_t waitCounter = 0;
  while (consumer.consumed.load(std::memory_order_relaxed) < produced) {
    if (++waitCounter > 1000000) {
      throw std::runtime_error("Failed to benchmark LfqRunner");
    }
    asm volatile("pause" ::: "memory");
  }
  lfqRunner.stop();
}
BENCHMARK(BM_Op_LfqRunner);

static void BM_Op_StreamBusPost(benchmark::State &state) {
  const auto order = utils::generateOrder();

  utils::pinThreadToCore(2);

  uint64_t produced = 0;
  std::atomic_uint64_t consumed;

  SystemBus systemBus;
  StreamBus<Order> streamBus{Config::get<CoreId>("bench.stream_core"), systemBus};

  streamBus.subscribe<Order>([&consumed](CRef<Order> o) {
    consumed.fetch_add(1, std::memory_order_relaxed);
    o.partialFill(1);
  });
  streamBus.run();

  for (auto _ : state) {
    auto tid = (uint32_t)order.ticker[0];
    benchmark::DoNotOptimize(tid);

    if (!streamBus.post<Order>(order)) {
      asm volatile("pause" ::: "memory");
    }
    ++produced;
    benchmark::DoNotOptimize(&order);
  }
  uint64_t waitCycles = 0;
  while (consumed.load(std::memory_order_relaxed) < produced) {
    if (++waitCycles > 10000000) {
      throw std::runtime_error(std::format("BM_Op_StreamBusPost produced: {} consumed: {}",
                                           produced, consumed.load(std::memory_order_relaxed)));
    }
    asm volatile("pause" ::: "memory");
  }

  streamBus.stop();
}
BENCHMARK(BM_Op_StreamBusPost);

} // namespace hft::benchmarks
