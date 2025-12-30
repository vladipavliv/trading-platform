/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include "bench_server.hpp"
#include "config/config.hpp"

namespace hft::benchmarks {

BM_Sys_ServerFix::BM_Sys_ServerFix() : orders{tickers}, marketData{tickers} {
  server::ServerConfig::load("bench_server_config.ini");

  tickerCount = Config::get<size_t>("bench.ticker_count");
  orderLimit = 16384 * 128;

  tickers.generate(tickerCount);
  orders.generate(orderLimit);
  marketData.generate(workerCount);

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

  setupCoordinator();
}

void BM_Sys_ServerFix::TearDown(const ::benchmark::State &state) {
  if (coordinator) {
    coordinator->stop();
    coordinator.reset();
  }
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

void BM_Sys_ServerFix::setupCoordinator() {
  using namespace server;

  ThreadId id = 0;
  for (auto &tkrData : marketData.marketData) {
    tkrData.second.setThreadId(id);
    if (++id == workerCount) {
      id = 0;
    }
  }

  flag.clear();
  coordinator = std::make_unique<Coordinator>(*bus, marketData.marketData);
  coordinator->start();
  flag.wait(false);
}

BENCHMARK_DEFINE_F(BM_Sys_ServerFix, AsyncProcess)(benchmark::State &state) {
  using namespace server;

  state.SetLabel(std::to_string(state.range(0)) + " worker(s)");

  if (!ServerConfig::cfg.coresNetwork.empty()) {
    utils::pinThreadToCore(ServerConfig::cfg.coresNetwork.front());
  }

  const uint64_t ordersCount = orders.orders.size();

  alignas(64) std::atomic<uint64_t> processed;
  alignas(64) std::atomic_flag signal;

  bus->subscribe<ServerOrderStatus>([&processed, &signal, ordersCount](CRef<ServerOrderStatus> s) {
    if (s.orderStatus.state == OrderState::Accepted &&
        processed.fetch_add(1, std::memory_order_relaxed) + 1 == ordersCount) {
      signal.test_and_set(std::memory_order_release);
    } else if (s.orderStatus.state == OrderState::Rejected) {
      throw std::runtime_error("Increase OrderBook limit");
    }
  });

  while (state.KeepRunningBatch(ordersCount)) {
    processed = 0;
    signal.clear();

    for (const auto &order : orders.orders) {
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
