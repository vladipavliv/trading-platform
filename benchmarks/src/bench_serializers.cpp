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
  return OrderTimestamp{utils::generateOrderId(), utils::getTimestamp(), utils::getTimestamp(),
                        utils::getTimestamp()};
}
} // namespace

static void BM_Ser_ProtoSerialize(benchmark::State &state) {
  const OrderTimestamp ot{generateOrderTimestamp()};
  String data;

  for (auto _ : state) {
    data = proto::ProtoMetadataSerializer::serialize(ot);
  }

  benchmark::DoNotOptimize(&data);
  benchmark::DoNotOptimize(&ot);
}
BENCHMARK(BM_Ser_ProtoSerialize);

static void BM_Ser_ProtoDeserialize(benchmark::State &state) {
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
BENCHMARK(BM_Ser_ProtoDeserialize);

static void BM_Ser_FbsSerialize(benchmark::State &state) {
  const Order order = utils::generateOrder();
  fbs::FbsDomainSerializer::BufferType buffer;

  for (auto _ : state) {
    buffer = fbs::FbsDomainSerializer::serialize(order);
  }

  benchmark::DoNotOptimize(&order);
  benchmark::DoNotOptimize(&buffer);
}
BENCHMARK(BM_Ser_FbsSerialize);

static void BM_Ser_FbsDeserialize(benchmark::State &state) {
  using BusType = BusHolder<MessageBus<Order>>;

  BusType bus;
  bus.template subscribe<Order>([](CRef<Order> o) {});

  const auto buffer = fbs::FbsDomainSerializer::serialize(utils::generateOrder());
  bool ok{false};

  for (auto _ : state) {
    ok = fbs::FbsDomainSerializer::deserialize<BusType>(buffer.data(), buffer.size(), bus);
  }
  benchmark::DoNotOptimize(&buffer);
  benchmark::DoNotOptimize(ok);
}
BENCHMARK(BM_Ser_FbsDeserialize);

static void BM_Ser_SbeSerialize(benchmark::State &state) {
  const Order order = utils::generateOrder();
  Vector<uint8_t> buffer;

  for (auto _ : state) {
    sbe::SbeDomainSerializer::serialize(order, buffer);
  }

  benchmark::DoNotOptimize(&order);
  benchmark::DoNotOptimize(&buffer);
}
BENCHMARK(BM_Ser_SbeSerialize);

static void BM_Ser_SbeDeserialize(benchmark::State &state) {
  using BusType = BusHolder<MessageBus<Order>>;

  const Order order = utils::generateOrder();
  ByteBuffer buffer;

  BusType bus;
  bus.template subscribe<Order>([](CRef<Order> o) {});

  sbe::SbeDomainSerializer::serialize(order, buffer);
  bool ok{false};

  for (auto _ : state) {
    ok = sbe::SbeDomainSerializer::deserialize(buffer.data(), buffer.size(), bus);
  }
  benchmark::DoNotOptimize(&buffer);
  benchmark::DoNotOptimize(ok);
}
BENCHMARK(BM_Ser_SbeDeserialize);

} // namespace hft::benchmarks
