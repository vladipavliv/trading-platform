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

namespace {
server::ServerOrder generateOrder() {
  using namespace utils;
  using namespace server;
  return ServerOrder{
      0, Order{generateOrderId(), getTimestamp(), "GGL", RNG::generate<Quantity>(0, 1000),
               RNG::generate<Price>(10, 10000),
               RNG::generate<uint8_t>(0, 1) == 0 ? OrderAction::Buy : OrderAction::Sell}};
}
} // namespace

class OrderBookFixture : public benchmark::Fixture {
public:
  server::Bus bus;
  UPtr<server::OrderBook> book;
  size_t orderLimit;
  std::vector<server::ServerOrder> orders;

  OrderBookFixture() {
    using namespace server;

    ServerConfig::load("bench_server_config.ini");
    resetOrderBook();
    orderLimit = ServerConfig::cfg.orderBookLimit;

    bus.marketBus.setHandler<ServerOrder>([](CRef<ServerOrder> o) {});
    bus.marketBus.setHandler<ServerOrderStatus>([](CRef<ServerOrderStatus> s) {});
    bus.marketBus.setHandler<TickerPrice>([](CRef<TickerPrice> p) {});

    // pregenerate a bunch of orders
    orders.reserve(orderLimit);
    for (size_t i = 0; i < orderLimit; ++i) {
      orders.emplace_back(generateOrder());
    }
  }

  void SetUp(const ::benchmark::State &) override {}

  void TearDown(const ::benchmark::State &) override {}

  void resetOrderBook() {
    using namespace server;
    book = std::make_unique<OrderBook>(bus);
  }
};

BENCHMARK_F(OrderBookFixture, AddOrder)(benchmark::State &state) {
  using namespace server;
  std::vector<server::ServerOrder>::iterator iter = orders.begin();

  for (auto _ : state) {
    if (iter == orders.end()) {
      iter = orders.begin();
    }

    CRef<ServerOrder> order = *iter++;
    if (book->openedOrders() >= ServerConfig::cfg.orderBookLimit) {
      resetOrderBook();
    }
    bool added = book->add(order);
    if (!added) {
      resetOrderBook();
      added = book->add(order);
    }
    book->match();

    benchmark::DoNotOptimize(&order);
    benchmark::DoNotOptimize(&added);
  }
}

} // namespace hft::benchmarks
