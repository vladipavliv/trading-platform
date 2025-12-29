/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include "bench_server.hpp"
#include "config/config.hpp"

namespace hft::benchmarks {

BM_Sys_ServerFix::BM_Sys_ServerFix() {
  server::ServerConfig::load("bench_server_config.ini");

  tickerCount = Config::get<size_t>("bench.ticker_count");
  orderLimit = server::ServerConfig::cfg.orderBookLimit;

  setupBus();
}

BM_Sys_ServerFix::~BM_Sys_ServerFix() { bus->stop(); }

void BM_Sys_ServerFix::SetUp(const ::benchmark::State &state) {
  using namespace server;

  workerCount = state.range(0);

  if (workerCount > 4) {
    throw std::runtime_error("Too many workers");
  }

  ServerConfig::cfg.coresApp.clear();
  ServerConfig::cfg.coresNetwork.clear();

  ServerConfig::cfg.coreSystem = 12;
  ServerConfig::cfg.coresNetwork.push_back(2);

  for (size_t i = 0; i < workerCount; ++i) {
    ServerConfig::cfg.coresApp.push_back(4 + (i * 2));
  }

  fillMarketData();
  setupCoordinator();
  fillOrders();
}

void BM_Sys_ServerFix::TearDown(const ::benchmark::State &state) {
  if (coordinator) {
    coordinator->stop();
    coordinator.reset();
  }
  cleanup();
}

void BM_Sys_ServerFix::setupBus() {
  using namespace server;

  bus = std::make_unique<ServerBus>();
  bus->systemBus.subscribe<ServerEvent>([this](CRef<ServerEvent> event) {
    if (event.state == ServerState::Operational) {
      flag.test_and_set();
      flag.notify_all();
    }
  });

  systemThread = std::jthread([this]() { bus->run(); });
}

void BM_Sys_ServerFix::cleanup() {
  marketData.clear();
  tickers.clear();
  coordinator.reset();
}

void BM_Sys_ServerFix::fillMarketData() {
  using namespace server;
  using namespace utils;

  marketData.reserve(tickerCount);
  tickers.reserve(tickerCount);

  size_t workerId{0};
  for (size_t i = 0; i < tickerCount; ++i) {
    const auto ticker = generateTicker();
    tickers.emplace_back(ticker);
    marketData.emplace(ticker, workerId);
    if (++workerId == workerCount) {
      workerId = 0;
    }
  }
}

void BM_Sys_ServerFix::setupCoordinator() {
  using namespace server;

  flag.clear();
  coordinator = std::make_unique<Coordinator>(*bus, marketData);
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
    auto order = utils::generateOrder(*dataIt++);
    order.id = i;
    orders.push_back(ServerOrder{0, order});
  }
}

BENCHMARK_DEFINE_F(BM_Sys_ServerFix, AsyncProcess)(benchmark::State &state) {
  using namespace server;

  state.SetLabel(std::to_string(state.range(0)) + " worker(s)");

  if (!ServerConfig::cfg.coresNetwork.empty()) {
    utils::pinThreadToCore(ServerConfig::cfg.coresNetwork.front());
  }

  const uint64_t ordersCount = orders.size();

  while (state.KeepRunningBatch(ordersCount)) {
    state.PauseTiming();

    alignas(64) std::atomic<uint64_t> processed;
    alignas(64) std::atomic_flag signal;

    bus->subscribe<ServerOrderStatus>([&](CRef<ServerOrderStatus> s) {
      if (s.orderStatus.state == OrderState::Accepted &&
          processed.fetch_add(1, std::memory_order_relaxed) + 1 == ordersCount) {
        signal.test_and_set(std::memory_order_release);
      }
    });
    state.ResumeTiming();

    for (const auto &order : orders) {
      bus->post(order);
    }

    while (!signal.test(std::memory_order_acquire)) {
      asm volatile("pause" ::: "memory");
    }
    benchmark::DoNotOptimize(processed);
    benchmark::DoNotOptimize(signal);
  }
}

BENCHMARK_REGISTER_F(BM_Sys_ServerFix, AsyncProcess)
    ->Arg(1)
    ->Arg(2)
    ->Arg(3)
    ->Arg(4)
    ->Unit(benchmark::kNanosecond);

} // namespace hft::benchmarks
