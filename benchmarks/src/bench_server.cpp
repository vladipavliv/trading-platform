/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include "bench_server.hpp"

namespace hft::benchmarks {

void ServerFixture::GlobalSetUp() {
  using namespace server;
  try {
    ServerConfig::load("bench_server_config.ini");
    LOG_INIT(ServerConfig::cfg.logOutput);

    orderLimit = ServerConfig::cfg.orderBookLimit;

    setupBus();
    fillMarketData();
    setupCoordinator();
    fillOrders();

    LOG_DEBUG("ServerFixture was set up");
  } catch (const std::exception &e) {
    LOG_DEBUG("std::exception {}", e.what());
  } catch (...) {
    LOG_DEBUG("unknown exception");
  }
}

void ServerFixture::GlobalTearDown() {
  coordinator->stop();
  bus->stop();
}

void ServerFixture::SetUp(::benchmark::State &state) {
  if (!bus) {
    GlobalSetUp();
  }
  if (!bus) {
    state.SkipWithError("Server not initialized");
    return;
  }
}

void ServerFixture::TearDown(const ::benchmark::State &) {}

void ServerFixture::fillMarketData() {
  using namespace server;
  using namespace utils;

  size_t workerId{0};
  for (size_t i = 0; i < 10; ++i) {
    const auto ticker = generateTicker();
    const auto price = RNG::generate<Price>(0, 100);
    data.emplace(ticker, std::make_unique<TickerData>(*bus, workerId, price));
    workerId = ++workerId == ServerConfig::cfg.coresApp.size() ? 0 : workerId;
  }
}

void ServerFixture::fillOrders() {
  using namespace server;
  auto dataIt = data.begin();

  // pregenerate a bunch of orders
  orders.reserve(orderLimit);
  for (size_t i = 0; i < orderLimit; ++i) {
    if (dataIt == data.end()) {
      dataIt = data.begin();
    }
    orders.emplace_back(ServerOrder{0, utils::generateOrder(dataIt++->first)});
  }
}

void ServerFixture::setupBus() {
  using namespace server;

  bus = std::make_unique<Bus>();
  bus->marketBus.setHandler<ServerOrderStatus>([](CRef<ServerOrderStatus> s) {
    ServerFixture::flag.clear();
    ServerFixture::flag.notify_all();
  });
  bus->systemBus.subscribe(ServerEvent::Operational, [] {
    LOG_INFO("Coordinator is ready for benchmarking");
    ServerFixture::flag.clear();
    ServerFixture::flag.notify_all();
  });
  systemThread = std::jthread([]() { ServerFixture::bus->run(); });
}

void ServerFixture::setupCoordinator() {
  using namespace server;

  flag.test_and_set();
  coordinator = std::make_unique<Coordinator>(*bus, data);
  coordinator->start();
  flag.wait(true);
}

BENCHMARK_F(ServerFixture, ProcessOrders)(benchmark::State &state) {
  using namespace server;

  if (ServerFixture::bus == nullptr) {
    state.SkipWithError("Server not initialized");
    return;
  }

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
