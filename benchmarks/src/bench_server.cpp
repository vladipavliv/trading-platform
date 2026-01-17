/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include "bench_server.hpp"
#include "config/config.hpp"
#include "config/server_config.hpp"
#include "internal_error.hpp"
#include "logging.hpp"
#include "schema.hpp"
#include "utils/spin_wait.hpp"
#include "utils/test_utils.hpp"

namespace hft::benchmarks {

using namespace server;
using namespace tests;
using namespace utils;

BM_ServerFix::BM_ServerFix() : orders{tickers}, marketData{tickers} {
  ServerConfig::cfg().load("bench_server_config.ini");
  LOG_INIT(ServerConfig::cfg().logOutput);

  tickerCount = Config::get<size_t>("bench.ticker_count");

  tickers.gen(tickerCount);
  orders.gen(MAX_BOOK_ORDERS);
  marketData.gen(workerCount);

  setupBus();
}

BM_ServerFix::~BM_ServerFix() {
  if (bus) {
    bus->stop();
    bus.reset();
  }
}

void BM_ServerFix::SetUp(const ::benchmark::State &state) {
  workerCount = state.range(0);
  if (workerCount > 4) {
    throw std::runtime_error("Too many workers");
  }

  ServerConfig::cfg().coresApp.clear();
  ServerConfig::cfg().coreNetwork = getCore(0);

  for (size_t i = 1; i <= workerCount; ++i) {
    ServerConfig::cfg().coresApp.push_back(getCore(i));
  }

  setupCoordinator();
}

void BM_ServerFix::TearDown(const ::benchmark::State &state) {
  if (coordinator) {
    coordinator->stop();
    coordinator.reset();
  }
}

void BM_ServerFix::setupBus() {
  using namespace server;
  bus = std::make_unique<ServerBus>();
  bus->systemBus.subscribe<ServerEvent>(
      CRefHandler<ServerEvent>::bind<BM_ServerFix, &BM_ServerFix::post>(this));
  systemThread = std::jthread([this]() { bus->run(); });
}

void BM_ServerFix::setupCoordinator() {
  using namespace server;

  ThreadId id = 0;
  for (auto &tkrData : marketData.marketData) {
    tkrData.second.workerId = id;
    if (++id == workerCount) {
      id = 0;
    }
  }

  flag.clear();
  coordinator = std::make_unique<Coordinator>(*bus, marketData.marketData);
  coordinator->start();
  flag.wait(false);
}

void BM_ServerFix::post(const ServerEvent &ev) {
  if (ev.state == ServerState::Operational) {
    flag.test_and_set();
    flag.notify_all();
  }
}

void BM_ServerFix::post(CRef<InternalOrderStatus> s) {
  if (s.state == OrderState::Rejected) {
    error.store(true, std::memory_order_release);
    LOG_ERROR_SYSTEM("Increase OrderBook limit");
  } else {
    processed.fetch_add(1, std::memory_order_relaxed);
  }
}

BENCHMARK_DEFINE_F(BM_ServerFix, InternalThroughput)(benchmark::State &state) {
  using namespace server;

  state.SetLabel(std::to_string(state.range(0)) + " worker(s)");
  const uint64_t ordersCount = orders.orders.size();

  bus->subscribe<InternalOrderStatus>(
      CRefHandler<InternalOrderStatus>::bind<BM_ServerFix, &BM_ServerFix::post>(this));

  SpinWait waiter{SPIN_RETRIES_YIELD};
  while (state.KeepRunningBatch(ordersCount)) {
    if (error.load(std::memory_order_acquire)) {
      state.SkipWithError("Increase OrderBook limit");
      break;
    }
    processed.store(0, std::memory_order_release);

    for (const auto &order : orders.orders) {
      benchmark::DoNotOptimize(&order);
      bus->post(order);
    }
    benchmark::ClobberMemory();

    while (processed.load(std::memory_order_relaxed) < ordersCount) {
      if (error.load(std::memory_order_acquire)) {
        state.SkipWithError("Increase OrderBook limit");
        break;
      }
      if (!++waiter) {
        break;
      }
    }

    if (processed.load(std::memory_order_relaxed) < ordersCount) {
      state.SkipWithError(std::format("Processed {} out if {}", processed.load(), ordersCount));
      break;
    }

    state.PauseTiming();
    marketData.cleanup();
    state.ResumeTiming();

    waiter.reset();
  }

  coordinator->stop();
  coordinator.reset();
}

BENCHMARK_DEFINE_F(BM_ServerFix, InternalLatency)(benchmark::State &state) {
  using namespace server;
  state.SetLabel(std::to_string(state.range(0)) + " worker(s)");

  const uint64_t ordersCount = orders.orders.size();

  bus->subscribe<InternalOrderStatus>(
      CRefHandler<InternalOrderStatus>::bind<BM_ServerFix, &BM_ServerFix::post>(this));

  SpinWait waiter;
  auto iter = orders.orders.begin();
  for (auto _ : state) {
    waiter.reset();
    if (iter == orders.orders.end()) {
      iter = orders.orders.begin();

      state.PauseTiming();
      marketData.cleanup();
      state.ResumeTiming();
    }

    benchmark::DoNotOptimize(&*iter);
    bus->post(*iter++);
    benchmark::ClobberMemory();

    uint32_t cycles = 0;
    while (!error.load(std::memory_order_acquire) &&
           processed.load(std::memory_order_relaxed) == 0) {
      if (!++waiter) {
        break;
      }
    }
    if (processed.load(std::memory_order_relaxed) == 0) {
      state.SkipWithError("Failed to process order in time");
    }
    processed.store(0, std::memory_order_relaxed);
  }

  benchmark::DoNotOptimize(processed);

  coordinator->stop();
  coordinator.reset();
}

BENCHMARK_REGISTER_F(BM_ServerFix, InternalThroughput)
    ->Arg(1)
    ->Arg(2)
    ->Arg(3)
    ->Arg(4)
    ->Unit(benchmark::kNanosecond);

BENCHMARK_REGISTER_F(BM_ServerFix, InternalLatency)
    ->Arg(1)
    ->Arg(2)
    ->Arg(3)
    ->Arg(4)
    ->Unit(benchmark::kNanosecond);

} // namespace hft::benchmarks
