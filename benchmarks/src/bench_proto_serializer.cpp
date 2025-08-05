/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include <benchmark/benchmark.h>

#include "config/server_config.hpp"
#include "execution/order_book.hpp"
#include "metadata_types.hpp"
#include "serialization/protobuf/proto_metadata_serializer.hpp"
#include "server_types.hpp"
#include "types.hpp"
#include "utils/rng.hpp"
#include "utils/utils.hpp"

namespace hft::benchmarks {

namespace {
OrderTimestamp generateOrderTimestamp() {
  return OrderTimestamp{utils::generateOrderId(), utils::getTimestamp(), utils::getTimestamp(),
                        utils::getTimestamp()};
}
} // namespace

static void BM_Ser_ProtoSerialize(benchmark::State &state) {
  using namespace utils;
  using namespace server;
  using namespace serialization;

  const OrderTimestamp ot{generateOrderTimestamp()};
  String data;

  for (auto _ : state) {
    data = ProtoMetadataSerializer::serialize(ot);
  }

  benchmark::DoNotOptimize(&data);
  benchmark::DoNotOptimize(&ot);
}
BENCHMARK(BM_Ser_ProtoSerialize);

static void BM_Ser_ProtoDeserialize(benchmark::State &state) {
  using namespace utils;
  using namespace server;
  using namespace serialization;

  using Bustype = BusHolder<OrderTimestamp>;

  Bustype bus;
  bus.marketBus.setHandler<OrderTimestamp>([](CRef<OrderTimestamp> o) {});

  const auto msg = ProtoMetadataSerializer::serialize(generateOrderTimestamp());
  bool ok{false};

  for (auto _ : state) {
    ok = ProtoMetadataSerializer::deserialize<Bustype>(
        reinterpret_cast<const uint8_t *>(msg.data()), msg.size(), bus);
  }
  benchmark::DoNotOptimize(&msg);
  benchmark::DoNotOptimize(&ok);
}
BENCHMARK(BM_Ser_ProtoDeserialize);

} // namespace hft::benchmarks
