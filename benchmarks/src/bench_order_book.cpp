/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include <benchmark/benchmark.h>

#include "bus/busable.hpp"
#include "config/server_config.hpp"
#include "container_types.hpp"
#include "domain/server_order_messages.hpp"
#include "execution/order_book.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"
#include "utils/id_utils.hpp"
#include "utils/rng.hpp"
#include "utils/test_utils.hpp"

namespace hft::benchmarks {

using namespace server;

class BM_Sys_OrderBookFix : public benchmark::Fixture {
public:
  uint64_t counter;

  inline static Vector<ServerOrder> orders;

  BM_Sys_OrderBookFix() { ServerConfig::load("bench_server_config.ini"); }

  template <typename EventType>
  void post(CRef<EventType>) {}

  void SetUp(const ::benchmark::State &) override {
    const size_t ordersCount = 16384;

    orders.reserve(ordersCount);
    for (size_t i = 0; i < ordersCount; ++i) {
      orders.emplace_back(ServerOrder{0, utils::generateOrder()});
    }
  }

  void TearDown(const ::benchmark::State &) override { orders.clear(); }
};

template <>
void BM_Sys_OrderBookFix::post<ServerOrderStatus>(CRef<ServerOrderStatus> event) {
  if (event.orderStatus.state == OrderState::Rejected) {
    throw std::runtime_error("Increase OrderBook limit");
  }
  ++counter;
}

BENCHMARK_F(BM_Sys_OrderBookFix, AddOrder)(benchmark::State &state) {
  OrderBook book;
  while (state.KeepRunningBatch(orders.size())) {
    book.clear();

    for (auto &order : orders) {
      if (book.add(order, *this)) {
        book.match(*this);
      }
    }
    benchmark::DoNotOptimize(counter);
  }
}

} // namespace hft::benchmarks
