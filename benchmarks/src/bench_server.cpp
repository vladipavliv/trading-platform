/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include "bench_server.hpp"
#include "config/config.hpp"

namespace hft::benchmarks {

BM_Sys_ServerFix::BM_Sys_ServerFix() { GlobalSetUp(); }

void BM_Sys_ServerFix::GlobalSetUp() {
  using namespace server;
  std::call_once(initFlag, []() {
    ServerConfig::load("bench_server_config.ini");
    LOG_INIT(ServerConfig::cfg.logOutput);

    tickerCount = Config::get<size_t>("bench.ticker_count");
    orderLimit = ServerConfig::cfg.orderBookLimit;
    workerCount = ServerConfig::cfg.coresApp.size() == 0 ? 1 : ServerConfig::cfg.coresApp.size();

    LOG_DEBUG("Tickers: {} Orders:{} Workers:{}", tickerCount, orderLimit, workerCount);

    setupBus();
    fillMarketData();
    setupCoordinator();
    fillOrders();
  });
}

void BM_Sys_ServerFix::GlobalTearDown() {
  coordinator->stop();
  bus->stop();
}

void BM_Sys_ServerFix::fillMarketData() {
  using namespace server;
  using namespace utils;

  data = std::make_unique<MarketData>(tickerCount);
  tickers.reserve(tickerCount);

  size_t workerId{0};
  for (size_t i = 0; i < tickerCount; ++i) {
    const auto ticker = generateTicker();
    tickers.emplace_back(ticker);
    data->emplace(ticker, workerId);
    if (++workerId == workerCount) {
      workerId = 0;
    }
  }
}

void BM_Sys_ServerFix::fillOrders() {
  using namespace server;
  auto dataIt = tickers.begin();

  // pregenerate a bunch of orders
  const size_t orderCount = orderLimit * workerCount * tickerCount;
  orders.reserve(orderCount);
  for (size_t i = 0; i < orderCount; ++i) {
    if (dataIt == tickers.end()) {
      dataIt = tickers.begin();
    }
    orders.emplace_back(ServerOrder{0, utils::generateOrder(*dataIt++)});
  }
}

void BM_Sys_ServerFix::setupBus() {
  using namespace server;

  bus = std::make_unique<Bus>();
  bus->marketBus.setHandler<ServerOrderStatus>([](CRef<ServerOrderStatus> s) {
    if (s.orderStatus.state == OrderState::Rejected) {
      throw std::runtime_error("Increase OrderBook limit");
    }
    BM_Sys_ServerFix::flag.clear();
    BM_Sys_ServerFix::flag.notify_all();
  });
  bus->systemBus.subscribe(ServerEvent::Operational, [] {
    LOG_INFO("Coordinator is ready for benchmarking");
    BM_Sys_ServerFix::flag.clear();
    BM_Sys_ServerFix::flag.notify_all();
  });
  systemThread = std::jthread([]() { BM_Sys_ServerFix::bus->run(); });
}

void BM_Sys_ServerFix::setupCoordinator() {
  using namespace server;

  flag.test_and_set();
  coordinator = std::make_unique<Coordinator>(*bus, *data);
  coordinator->start();
  flag.wait(true);
}

void BM_Sys_ServerFix::cleanupOrders() {
  for (auto &ticker : tickers) {
    data->at(ticker).orderBook.extract();
  }
}

BENCHMARK_F(BM_Sys_ServerFix, ProcessOrders)(benchmark::State &state) {
  using namespace server;

  auto iter = orders.begin();
  for (auto _ : state) {
    if (iter == orders.end()) {
      iter = orders.begin();
    }
    CRef<ServerOrder> order = *iter++;

    flag.test_and_set();
    bus->post(order);
    flag.wait(true);

    benchmark::DoNotOptimize(flag);
  }
}

} // namespace hft::benchmarks
