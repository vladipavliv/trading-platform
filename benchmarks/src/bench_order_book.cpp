/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include <benchmark/benchmark.h>

#include "concepts/busable.hpp"
#include "config/server_config.hpp"
#include "execution/order_book.hpp"
#include "server_types.hpp"
#include "types.hpp"
#include "utils/rng.hpp"
#include "utils/utils.hpp"

namespace hft::benchmarks {

class OrderBookFixture : public benchmark::Fixture {
public:
  UPtr<server::OrderBook> book;
  size_t orderLimit;
  std::vector<server::ServerOrder> orders;

  OrderBookFixture() {
    using namespace server;

    ServerConfig::load("bench_server_config.ini");
    book = std::make_unique<OrderBook>();
    orderLimit = ServerConfig::cfg.orderBookLimit;

    // pregenerate a bunch of orders
    orders.reserve(orderLimit);
    for (size_t i = 0; i < orderLimit; ++i) {
      orders.emplace_back(ServerOrder{0, utils::generateOrder()});
    }
  }

  template <typename EventType>
  void post(CRef<EventType>) {}

  void SetUp(const ::benchmark::State &) override {}

  void TearDown(const ::benchmark::State &) override {}
};

template <>
void OrderBookFixture::post<server::ServerOrderStatus>(CRef<server::ServerOrderStatus> event) {
  if (event.orderStatus.state == OrderState::Rejected) {
    book->extract();
  }
}

BENCHMARK_F(OrderBookFixture, AddOrder)(benchmark::State &state) {
  using namespace server;
  std::vector<server::ServerOrder>::iterator iter = orders.begin();

  for (auto _ : state) {
    if (iter == orders.end()) {
      iter = orders.begin();
    }

    CRef<ServerOrder> order = *iter++;
    bool added = book->add(order, *this);
    book->match(*this);

    benchmark::DoNotOptimize(&order);
    benchmark::DoNotOptimize(&added);
  }
}

} // namespace hft::benchmarks
