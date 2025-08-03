/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include <benchmark/benchmark.h>

#include "config/server_config.hpp"
#include "domain_types.hpp"
#include "serialization/flat_buffers/fbs_domain_serializer.hpp"
#include "serialization/flat_buffers/fbs_metadata_serializer.hpp"
#include "server_types.hpp"
#include "types.hpp"
#include "utils/rng.hpp"
#include "utils/utils.hpp"

namespace hft::benchmarks {

static void BM_fbsSerialize(benchmark::State &state) {
  using namespace server;
  using namespace serialization;

  const Order order = utils::generateOrder();
  FbsDomainSerializer::BufferType buffer;

  for (auto _ : state) {
    buffer = FbsDomainSerializer::serialize(order);
  }

  benchmark::DoNotOptimize(&order);
  benchmark::DoNotOptimize(&buffer);
}
BENCHMARK(BM_fbsSerialize);

static void BM_fbsDeserialize(benchmark::State &state) {
  using namespace server;
  using namespace serialization;

  BusHolder<Order> bus;
  bus.marketBus.setHandler<Order>([](CRef<Order> o) {});

  const auto buffer = FbsDomainSerializer::serialize(utils::generateOrder());
  bool ok{false};

  for (auto _ : state) {
    ok = FbsDomainSerializer::deserialize<BusHolder<Order>>(buffer.data(), buffer.size(), bus);
  }
  benchmark::DoNotOptimize(&buffer);
  benchmark::DoNotOptimize(ok);
}
BENCHMARK(BM_fbsDeserialize);

} // namespace hft::benchmarks
