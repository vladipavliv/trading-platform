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

using namespace server;

class BM_Sys_OrderBookFix : public benchmark::Fixture {
public:
  UPtr<OrderBook> book;

  inline static Vector<ServerOrder> orders;
  inline static std::once_flag initFlag;

  BM_Sys_OrderBookFix() {
    std::call_once(initFlag, []() {
      ServerConfig::load("bench_server_config.ini");
      LOG_INIT(ServerConfig::cfg.logOutput);

      orders.reserve(ServerConfig::cfg.orderBookLimit);
      for (size_t i = 0; i < ServerConfig::cfg.orderBookLimit; ++i) {
        orders.emplace_back(ServerOrder{0, utils::generateOrder()});
      }
    });
    book = std::make_unique<OrderBook>();
  }

  template <typename EventType>
  void post(CRef<EventType>) {}

  void SetUp(const ::benchmark::State &) override {}

  void TearDown(const ::benchmark::State &) override {}
};

template <>
void BM_Sys_OrderBookFix::post<ServerOrderStatus>(CRef<ServerOrderStatus> event) {
  if (event.orderStatus.state == OrderState::Rejected) {
    book->extract();
  }
}

BENCHMARK_F(BM_Sys_OrderBookFix, AddOrder)(benchmark::State &state) {
  Vector<ServerOrder>::iterator iter = orders.begin();
  for (auto _ : state) {
    if (iter == orders.end()) {
      iter = orders.begin();
    }

    CRef<ServerOrder> order = *iter++;
    bool added = book->add(order, *this);
    if (added) {
      book->match(*this);
    }

    benchmark::DoNotOptimize(&order);
    benchmark::DoNotOptimize(added);
  }
}

} // namespace hft::benchmarks
