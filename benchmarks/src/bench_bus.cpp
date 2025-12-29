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

static void BM_Op_MessageBusPost(benchmark::State &state) {
  size_t counter{0};
  const auto order = utils::generateOrder();

  MessageBus<Order> bus;
  bus.subscribe<Order>([&counter](CRef<Order> order) { counter += order.id; });

  for (auto _ : state) {
    bus.post<Order>(order);
    benchmark::DoNotOptimize(&order);
    benchmark::DoNotOptimize(counter);
  }
}
BENCHMARK(BM_Op_MessageBusPost);

static void BM_Op_SystemBusPost(benchmark::State &state) {
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
BENCHMARK(BM_Op_SystemBusPost);

struct alignas(64) Matcher {
  alignas(64) std::atomic<uint64_t> consumed;

  template <typename Message>
  void process(const Message &msg) {
    consumed.fetch_add(1, std::memory_order_relaxed);
  }
};

static void BM_Op_LfqRunner(benchmark::State &state) {
  const auto order = utils::generateOrder();

  utils::pinThreadToCore(2);

  uint64_t produced = 0;
  Matcher matcher;
  SystemBus bus;

  LfqRunner<Order, Matcher> lfqRunner{4, matcher, ErrorBus{bus}};
  lfqRunner.run();

  for (auto _ : state) {
    lfqRunner.post(order);
    ++produced;
    benchmark::DoNotOptimize(produced);
  }

  size_t waitCounter = 0;
  while (matcher.consumed.load(std::memory_order_relaxed) < produced) {
    if (++waitCounter > 1000000) {
      throw std::runtime_error("Failed to benchmark LfqRunner");
    }
    asm volatile("pause" ::: "memory");
  }
  lfqRunner.stop();
}
BENCHMARK(BM_Op_LfqRunner);

static void DISABLED_BM_Op_StreamBusPost(benchmark::State &state) {
  const auto order = utils::generateOrder();

  SystemBus systemBus;
  StreamBus<Order> streamBus{systemBus};
  streamBus.subscribe<Order>([](CRef<Order> o) { o.partialFill(1); });
  streamBus.run();

  for (auto _ : state) {
    streamBus.post<Order>(order);
    benchmark::DoNotOptimize(&order);
  }
  streamBus.stop();
}
BENCHMARK(DISABLED_BM_Op_StreamBusPost);

} // namespace hft::benchmarks
