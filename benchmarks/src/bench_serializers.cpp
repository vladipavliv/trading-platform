/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include <benchmark/benchmark.h>

#include "bus/bus_hub.hpp"
#include "config/server_config.hpp"
#include "container_types.hpp"
#include "domain/server_order_messages.hpp"
#include "domain_types.hpp"
#include "execution/orderbook/flat_order_book.hpp"
#include "primitive_types.hpp"
#include "serialization/fbs/fbs_domain_serializer.hpp"
#include "serialization/sbe/sbe_domain_serializer.hpp"
#include "traits.hpp"
#include "utils/data_generator.hpp"
#include "utils/rng.hpp"

namespace hft::benchmarks {

using namespace utils;
using namespace server;
using namespace serialization;
using namespace tests;

ServerConfig cfg{"bench_server_config.ini"};

static void DISABLED_BM_FbsSerialize(benchmark::State &state) {
  const Order order = genOrder();
  ByteBuffer buffer(128);
  size_t size{0};

  for (auto _ : state) {
    size = fbs::FbsDomainSerializer::serialize(order, buffer.data());
  }

  benchmark::DoNotOptimize(size);
  benchmark::DoNotOptimize(&order);
  benchmark::DoNotOptimize(&buffer);
}
BENCHMARK(DISABLED_BM_FbsSerialize);

static void DISABLED_BM_FbsDeserialize(benchmark::State &state) {
  using BusType = BusHub<MessageBus<Order>>;

  BusType bus{cfg.data};
  bus.subscribe(CRefHandler<Order>{});

  ByteBuffer buffer(128);
  auto size = fbs::FbsDomainSerializer::serialize(genOrder(), buffer.data());

  for (auto _ : state) {
    auto res = fbs::FbsDomainSerializer::deserialize<BusType>(buffer.data(), buffer.size(), bus);
    assert(res);
    size = *res;
  }
  benchmark::DoNotOptimize(&buffer);
  benchmark::DoNotOptimize(size);
}
BENCHMARK(DISABLED_BM_FbsDeserialize);

static void DISABLED_BM_SbeSerialize(benchmark::State &state) {
  const Order order = genOrder();
  Vector<uint8_t> buffer(128);
  size_t size{0};

  for (auto _ : state) {
    size = sbe::SbeDomainSerializer::serialize(order, buffer.data());
  }

  benchmark::DoNotOptimize(size);
  benchmark::DoNotOptimize(&order);
  benchmark::DoNotOptimize(&buffer);
}
BENCHMARK(DISABLED_BM_SbeSerialize);

static void DISABLED_BM_SbeDeserialize(benchmark::State &state) {
  using BusType = BusHub<MessageBus<Order>>;

  const Order order = genOrder();
  ByteBuffer buffer(128);

  BusType bus{cfg.data};
  bus.subscribe(CRefHandler<Order>{});

  size_t size = sbe::SbeDomainSerializer::serialize(order, buffer.data());

  for (auto _ : state) {
    auto res = sbe::SbeDomainSerializer::deserialize(buffer.data(), buffer.size(), bus);
    assert(res);
    size = *res;
  }
  benchmark::DoNotOptimize(&buffer);
  benchmark::DoNotOptimize(size);
}
BENCHMARK(DISABLED_BM_SbeDeserialize);

} // namespace hft::benchmarks
