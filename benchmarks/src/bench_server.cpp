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

void BM_Sys_ServerFix::setupBus() {
  using namespace server;

  bus = std::make_unique<ServerBus>();
  bus->systemBus.subscribe<ServerEvent>([](CRef<ServerEvent> event) {
    if (event.state == ServerState::Operational) {
      LOG_INFO("Coordinator is ready for benchmarking");
      BM_Sys_ServerFix::flag.test_and_set();
      BM_Sys_ServerFix::flag.notify_all();
    }
  });

  systemThread = std::jthread([]() { BM_Sys_ServerFix::bus->run(); });
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

void BM_Sys_ServerFix::setupCoordinator() {
  using namespace server;

  flag.clear();
  coordinator = std::make_unique<Coordinator>(*bus, *data);
  coordinator->start();
  flag.wait(false);
}

void BM_Sys_ServerFix::fillOrders() {
  using namespace server;

  const size_t rawCount = orderLimit * tickerCount;
  size_t orderCount = 1;
  while (orderCount < rawCount) {
    orderCount <<= 1;
  }

  orders.clear();
  orders.reserve(orderCount);

  auto dataIt = tickers.begin();
  for (size_t i = 0; i < orderCount; ++i) {
    if (dataIt == tickers.end()) {
      dataIt = tickers.begin();
    }
    // Generate orders for all slots up to the power of two
    auto order = utils::generateOrder(*dataIt++);
    order.id = i;
    orders.push_back(ServerOrder{0, order});
  }
}

BENCHMARK_F(BM_Sys_ServerFix, AsyncProcess_1Worker)(benchmark::State &state) {
  using namespace server;

  if (!ServerConfig::cfg.coresNetwork.empty()) {
    utils::pinThreadToCore(ServerConfig::cfg.coresNetwork.front());
  }

  uint64_t sent = 0;
  alignas(64) std::atomic<uint64_t> processed = 0;

  bus->subscribe<ServerOrderStatus>([&sent, &processed](CRef<ServerOrderStatus> s) {
    if (s.orderStatus.state == OrderState::Accepted) {
      processed.fetch_add(1, std::memory_order_relaxed);
    }
  });

  auto iter = orders.begin();
  for (auto _ : state) {
    if (iter == orders.end()) {
      iter = orders.begin();
    }
    ServerOrder &order = *iter++;
    bus->post(order);
    ++sent;
  }

  while (processed.load(std::memory_order_relaxed) < sent) {
    asm volatile("pause" ::: "memory");
  }

  state.SetItemsProcessed(sent);
}

} // namespace hft::benchmarks
