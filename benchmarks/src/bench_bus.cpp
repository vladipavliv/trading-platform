/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include <benchmark/benchmark.h>

#include "bus/bus.hpp"
#include "domain_types.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft::benchmarks {

namespace {
struct OrderListener {
  size_t listen(CRef<Order> order) { return order.id; }
};

} // namespace

static void BM_messageBusPost(benchmark::State &state) {
  MessageBus<Order> bus;
  size_t counter{0};

  OrderListener listener;

  bus.setHandler<Order>(
      [&counter, &listener](CRef<Order> order) { counter += listener.listen(order); });

  for (auto _ : state) {
    bus.post<Order>(Order{});
  }
  benchmark::DoNotOptimize(counter);
}
BENCHMARK(BM_messageBusPost);

static void BM_systemBusPost(benchmark::State &state) {
  SystemBus bus;
  Order order;

  bus.subscribe<Order>([&order](CRef<Order> o) { o.partialFill(1); });

  for (auto _ : state) {
    bus.post<Order>(Order{});
  }
  benchmark::DoNotOptimize(order);
}
BENCHMARK(BM_systemBusPost);

} // namespace hft::benchmarks
