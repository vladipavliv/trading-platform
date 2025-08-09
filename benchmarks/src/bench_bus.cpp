/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include <benchmark/benchmark.h>

#include "bus/bus_holder.hpp"
#include "domain_types.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft::benchmarks {

namespace {
/**
 * @brief Dummy struct to capture in lambda
 */
struct OrderListener {
  size_t listen(CRef<Order> order) {
    counter += order.price;
    return order.quantity;
  }

  const String name{"listener"};
  size_t counter{0};
};

} // namespace

static void BM_Op_MessageBusPost(benchmark::State &state) {
  OrderListener listener;
  size_t counter{0};
  const auto order = utils::generateOrder();

  MessageBus<Order> bus;
  bus.subscribe<Order>(
      [&counter, &listener](CRef<Order> order) { counter += listener.listen(order); });

  for (auto _ : state) {
    bus.post<Order>(order);
    benchmark::DoNotOptimize(&order);
    benchmark::DoNotOptimize(counter);
  }
}
BENCHMARK(BM_Op_MessageBusPost);

static void BM_Op_SystemBusPost(benchmark::State &state) {
  const auto order = utils::generateOrder();

  SystemBus bus;
  bus.subscribe<Order>([](CRef<Order> o) { o.partialFill(1); });

  for (auto _ : state) {
    bus.post<Order>(order);
    benchmark::DoNotOptimize(&order);
  }
}
BENCHMARK(BM_Op_SystemBusPost);

static void BM_Op_DataBusPost(benchmark::State &state) {
  const auto order = utils::generateOrder();

  DataBus<Order> bus;
  bus.subscribe<Order>([](CRef<Order> o) { o.partialFill(1); });
  bus.run();

  for (auto _ : state) {
    bus.post<Order>(order);
    benchmark::DoNotOptimize(&order);
  }
  bus.stop();
}
BENCHMARK(BM_Op_DataBusPost);

} // namespace hft::benchmarks
