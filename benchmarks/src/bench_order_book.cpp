/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include <benchmark/benchmark.h>

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
    resetOrderBook();
    orderLimit = ServerConfig::cfg.orderBookLimit;

    // pregenerate a bunch of orders
    orders.reserve(orderLimit);
    for (size_t i = 0; i < orderLimit; ++i) {
      orders.emplace_back(ServerOrder{0, utils::generateOrder()});
    }
  }

  void SetUp(const ::benchmark::State &) override {}

  void TearDown(const ::benchmark::State &) override {}

  void resetOrderBook() {
    using namespace server;
    book = std::make_unique<OrderBook>();
  }
};

BENCHMARK_F(OrderBookFixture, AddOrder)(benchmark::State &state) {
  using namespace server;
  std::vector<server::ServerOrder>::iterator iter = orders.begin();

  const auto matcher = [](CRef<ServerOrderStatus> status) {};

  for (auto _ : state) {
    if (iter == orders.end()) {
      iter = orders.begin();
    }

    CRef<ServerOrder> order = *iter++;
    if (book->openedOrders() >= ServerConfig::cfg.orderBookLimit) {
      resetOrderBook();
    }
    bool added = book->add(order, matcher);
    if (!added) {
      resetOrderBook();
      added = book->add(order, matcher);
    }
    book->match(matcher);

    benchmark::DoNotOptimize(&order);
    benchmark::DoNotOptimize(&added);
  }
}

} // namespace hft::benchmarks
