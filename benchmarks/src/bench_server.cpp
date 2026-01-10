/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include "bench_server.hpp"
#include "config/config.hpp"
#include "config/server_config.hpp"
#include "internal_error.hpp"
#include "logging.hpp"
#include "utils/bench_utils.hpp"

namespace hft::benchmarks {

using namespace server;

BM_Sys_ServerFix::BM_Sys_ServerFix() : orders{tickers}, marketData{tickers} {
  ServerConfig::load("bench_server_config.ini");
  LOG_INIT(ServerConfig::cfg.logOutput);

  tickerCount = Config::get<size_t>("bench.ticker_count");
  orderLimit = Config::get<size_t>("bench.order_count");

  tickers.generate(tickerCount);
  orders.generate(orderLimit);
  marketData.generate(workerCount);

  setupBus();
}

BM_Sys_ServerFix::~BM_Sys_ServerFix() {
  if (bus) {
    bus->stop();
    bus.reset();
  }
}

void BM_Sys_ServerFix::SetUp(const ::benchmark::State &state) {

  workerCount = state.range(0);

  if (workerCount > 4) {
    throw std::runtime_error("Too many workers");
  }

  ServerConfig::cfg.coresApp.clear();

  ServerConfig::cfg.coreNetwork = getCore(0);

  for (size_t i = 1; i <= workerCount; ++i) {
    ServerConfig::cfg.coresApp.push_back(getCore(i));
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

  const uint64_t ordersCount = orders.orders.size();

  alignas(64) std::atomic<uint64_t> processed;
  alignas(64) AtomicUInt32 signal;

  bus->subscribe<ServerOrderStatus>(
      [this, &processed, &signal, ordersCount](CRef<ServerOrderStatus> s) {
        if (s.orderStatus.state == OrderState::Accepted &&
            processed.fetch_add(1, std::memory_order_release) + 1 == ordersCount) {
          signal.fetch_add(1, std::memory_order_release);
          utils::futexWake(signal);
        } else if (s.orderStatus.state == OrderState::Rejected) {
          error_.store(true, std::memory_order_release);
          LOG_ERROR_SYSTEM("Increase OrderBook limit");
        }
      });

  while (state.KeepRunningBatch(ordersCount)) {
    if (error_.load(std::memory_order_acquire)) {
      throw std::runtime_error("error");
    }
    processed.store(0, std::memory_order_release);

    for (const auto &order : orders.orders) {
      bus->post(order);
    }

    auto ftxVal = signal.load(std::memory_order_acquire);
    if (ftxVal < ordersCount) {
      utils::futexWait(signal, ftxVal, 100000000);
    }

    auto res = processed.load(std::memory_order_acquire);
    if (res < ordersCount) {
      throw std::runtime_error(std::format("Benchmark failed"));
    }
    marketData.cleanup();

    benchmark::DoNotOptimize(processed);
    benchmark::DoNotOptimize(signal);
  }
  coordinator->stop();
  coordinator.reset();
}

BENCHMARK_REGISTER_F(BM_Sys_ServerFix, AsyncProcess)
    ->Arg(1)
    ->Arg(2)
    ->Arg(3)
    ->Arg(4)
    ->Unit(benchmark::kNanosecond);

} // namespace hft::benchmarks
