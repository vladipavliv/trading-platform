/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include <benchmark/benchmark.h>

#include "config/server_config.hpp"
#include "domain_types.hpp"
#include "execution/order_book.hpp"
#include "metadata_types.hpp"
#include "serialization/fbs/fbs_domain_serializer.hpp"
#include "serialization/fbs/fbs_metadata_serializer.hpp"
#include "serialization/proto/proto_metadata_serializer.hpp"
#include "serialization/sbe/sbe_domain_serializer.hpp"
#include "server_types.hpp"
#include "types.hpp"
#include "utils/rng.hpp"
#include "utils/utils.hpp"

namespace hft::benchmarks {

using namespace utils;
using namespace server;
using namespace serialization;

namespace {
OrderTimestamp generateOrderTimestamp() {
  return OrderTimestamp{utils::generateOrderId(), utils::getTimestampNs(), utils::getTimestampNs(),
                        utils::getTimestampNs()};
}
} // namespace

static void DISABLED_BM_Ser_ProtoSerialize(benchmark::State &state) {
  const OrderTimestamp ot{generateOrderTimestamp()};
  String data;

  for (auto _ : state) {
    data = proto::ProtoMetadataSerializer::serialize(ot);
  }

  benchmark::DoNotOptimize(&data);
  benchmark::DoNotOptimize(&ot);
}
BENCHMARK(DISABLED_BM_Ser_ProtoSerialize);

static void DISABLED_BM_Ser_ProtoDeserialize(benchmark::State &state) {
  using Bustype = BusHolder<MessageBus<OrderTimestamp>>;

  Bustype bus;
  bus.template subscribe<OrderTimestamp>([](CRef<OrderTimestamp> o) {});

  const auto msg = proto::ProtoMetadataSerializer::serialize(generateOrderTimestamp());
  bool ok{false};

  for (auto _ : state) {
    ok = proto::ProtoMetadataSerializer::deserialize<Bustype>(
        reinterpret_cast<const uint8_t *>(msg.data()), msg.size(), bus);
  }
  benchmark::DoNotOptimize(&msg);
  benchmark::DoNotOptimize(&ok);
}
BENCHMARK(DISABLED_BM_Ser_ProtoDeserialize);

static void DISABLED_BM_Ser_FbsSerialize(benchmark::State &state) {
  const Order order = utils::generateOrder();
  ByteBuffer buffer(128);
  size_t size{0};

  for (auto _ : state) {
    size = fbs::FbsDomainSerializer::serialize(order, buffer.data());
  }

  benchmark::DoNotOptimize(size);
  benchmark::DoNotOptimize(&order);
  benchmark::DoNotOptimize(&buffer);
}
BENCHMARK(DISABLED_BM_Ser_FbsSerialize);

static void DISABLED_BM_Ser_FbsDeserialize(benchmark::State &state) {
  using BusType = BusHolder<MessageBus<Order>>;

  BusType bus;
  bus.template subscribe<Order>([](CRef<Order> o) {});

  ByteBuffer buffer(128);
  auto size = fbs::FbsDomainSerializer::serialize(utils::generateOrder(), buffer.data());

  for (auto _ : state) {
    size = fbs::FbsDomainSerializer::deserialize<BusType>(buffer.data(), buffer.size(), bus);
  }
  benchmark::DoNotOptimize(&buffer);
  benchmark::DoNotOptimize(size);
}
BENCHMARK(DISABLED_BM_Ser_FbsDeserialize);

static void DISABLED_BM_Ser_SbeSerialize(benchmark::State &state) {
  const Order order = utils::generateOrder();
  Vector<uint8_t> buffer(128);
  size_t size{0};

  for (auto _ : state) {
    size = sbe::SbeDomainSerializer::serialize(order, buffer.data());
  }

  benchmark::DoNotOptimize(size);
  benchmark::DoNotOptimize(&order);
  benchmark::DoNotOptimize(&buffer);
}
BENCHMARK(DISABLED_BM_Ser_SbeSerialize);

static void DISABLED_BM_Ser_SbeDeserialize(benchmark::State &state) {
  using BusType = BusHolder<MessageBus<Order>>;

  const Order order = utils::generateOrder();
  ByteBuffer buffer(128);

  BusType bus;
  bus.template subscribe<Order>([](CRef<Order> o) {});

  size_t size = sbe::SbeDomainSerializer::serialize(order, buffer.data());

  for (auto _ : state) {
    size = sbe::SbeDomainSerializer::deserialize(buffer.data(), buffer.size(), bus);
  }
  benchmark::DoNotOptimize(&buffer);
  benchmark::DoNotOptimize(size);
}
BENCHMARK(DISABLED_BM_Ser_SbeDeserialize);

} // namespace hft::benchmarks
