/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include <benchmark/benchmark.h>

#include "bus/busable.hpp"
#include "config/server_config.hpp"
#include "container_types.hpp"
#include "domain/server_order_messages.hpp"
#include "execution/orderbook/flat_order_book.hpp"
#include "execution/orderbook/price_level_order_book.hpp"
#include "gateway/internal_order.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"
#include "traits.hpp"
#include "utils/data_generator.hpp"
#include "utils/rng.hpp"

namespace hft::benchmarks {

using namespace server;
using namespace utils;
using namespace tests;

class BM_OrderBookFix : public benchmark::Fixture {
public:
  ServerConfig cfg;
  uint64_t counter;

  inline static Vector<InternalOrderEvent> orders;

  BM_OrderBookFix() : cfg{"bench_server_config.ini"} {}

  template <typename EventType>
  void post(CRef<EventType>) {}

  void SetUp(const ::benchmark::State &) override {
    const size_t ordersCount = 16384;

    orders.reserve(ordersCount);
    for (size_t i = 0; i < ordersCount; ++i) {
      auto io = genInternalOrder();
      orders.emplace_back(io);
    }
  }

  void TearDown(const ::benchmark::State &) override { orders.clear(); }
};

template <>
void BM_OrderBookFix::post<InternalOrderStatus>(CRef<InternalOrderStatus> s) {
  if (s.state == OrderState::Rejected) {
    throw std::runtime_error("Increase OrderBook limit");
  }
  ++counter;
}

BENCHMARK_F(BM_OrderBookFix, AddOrder)(benchmark::State &state) {
  OrderBook book;
  while (state.KeepRunningBatch(orders.size())) {
    book.clear();

    for (auto &order : orders) {
      book.add(order, *this);
    }
    benchmark::DoNotOptimize(counter);
  }
}

} // namespace hft::benchmarks
