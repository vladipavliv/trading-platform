/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include "bench_coordinator.hpp"
#include "config/config.hpp"
#include "config/server_config.hpp"
#include "internal_error.hpp"
#include "logging.hpp"
#include "schema.hpp"
#include "utils/handler.hpp"
#include "utils/spin_wait.hpp"
#include "utils/test_utils.hpp"

namespace hft::benchmarks {

using namespace server;
using namespace tests;
using namespace utils;

BM_CoordinatorFix::BM_CoordinatorFix()
    : cfg{"bench_server_config.ini"}, bus{cfg.data}, ctx{bus, cfg, stopSrc.get_token()} {
  LOG_INIT(cfg.data);

  orders.gen(MAX_BOOK_ORDERS);
  marketData.gen(workerCount);

  startBus();
}

BM_CoordinatorFix::~BM_CoordinatorFix() {
  stopSrc.request_stop();
  bus.stop();
}

void BM_CoordinatorFix::SetUp(const ::benchmark::State &state) {
  stopSrc = std::stop_source();
  ctx.stopToken = stopSrc.get_token();

  workerCount = state.range(0);
  if (workerCount > 4) {
    throw std::runtime_error("Too many workers");
  }

  cfg.coresApp.clear();
  cfg.coreNetwork = getCore(cfg.data, 0);

  for (size_t i = 1; i <= workerCount; ++i) {
    cfg.coresApp.push_back(getCore(cfg.data, i));
  }

  setupCoordinator();
}

void BM_CoordinatorFix::TearDown(const ::benchmark::State &state) {
  stopSrc.request_stop();
  if (coordinator) {
    coordinator->stop();
    coordinator.reset();
  }
}

void BM_CoordinatorFix::startBus() {
  bus.systemBus.subscribe(
      CRefHandler<ComponentReady>::bind<BM_CoordinatorFix, &BM_CoordinatorFix::post>(this));
  systemThread = std::jthread([this]() { bus.run(); });
}

void BM_CoordinatorFix::setupCoordinator() {
  ThreadId id = 0;
  for (auto &tkrData : marketData.marketData) {
    tkrData.workerId = id;
    if (++id == workerCount) {
      id = 0;
    }
  }

  flag.clear();
  coordinator = std::make_unique<Coordinator>(ctx, marketData.marketData);
  coordinator->start();
  flag.wait(false);
}

void BM_CoordinatorFix::post(const ComponentReady &ev) {
  flag.test_and_set();
  flag.notify_all();
}

void BM_CoordinatorFix::post(CRef<InternalOrderStatus> s) {
  // In the current flow only one status event is sent per order
  // if order triggers fulfills with resting orders, single partial/full event is sent
  // If order rests - accepted state event is sent
  // resting orders never generate state events
  if (s.state == OrderState::Rejected) {
    error.store(true, std::memory_order_release);
    LOG_ERROR_SYSTEM("Increase OrderBook limit");
  } else {
    // so all non-rejected events would give us overall processed orders count
    processed.fetch_add(1, std::memory_order_relaxed);
  }
}

BENCHMARK_DEFINE_F(BM_CoordinatorFix, Throughput)(benchmark::State &state) {
  state.SetLabel(std::to_string(state.range(0)) + " worker(s)");
  const uint64_t ordersCount = orders.orders.size();

  bus.subscribe(
      CRefHandler<InternalOrderStatus>::bind<BM_CoordinatorFix, &BM_CoordinatorFix::post>(this));

  SpinWait waiter{SPIN_RETRIES_YIELD};
  while (state.KeepRunningBatch(ordersCount)) {
    if (error.load(std::memory_order_acquire)) {
      state.SkipWithError("Increase OrderBook limit");
      break;
    }
    processed.store(0, std::memory_order_release);

    for (const auto &order : orders.orders) {
      benchmark::DoNotOptimize(&order);
      bus.post(order);
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
      state.SkipWithError(
          std::format("Processed {} out if {} orders", processed.load(), ordersCount));
      break;
    }

    state.PauseTiming();
    marketData.cleanup();
    state.ResumeTiming();

    waiter.reset();
  }
}

BENCHMARK_DEFINE_F(BM_CoordinatorFix, Latency)(benchmark::State &state) {
  state.SetLabel(std::to_string(state.range(0)) + " worker(s)");

  const uint64_t ordersCount = orders.orders.size();

  bus.subscribe(
      CRefHandler<InternalOrderStatus>::bind<BM_CoordinatorFix, &BM_CoordinatorFix::post>(this));

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
    bus.post(*iter++);
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
}

BENCHMARK_REGISTER_F(BM_CoordinatorFix, Throughput)
    ->Arg(1)
    ->Arg(2)
    ->Arg(3)
    ->Arg(4)
    ->Unit(benchmark::kNanosecond);

BENCHMARK_REGISTER_F(BM_CoordinatorFix, Latency)
    ->Arg(1)
    ->Arg(2)
    ->Arg(3)
    ->Arg(4)
    ->Unit(benchmark::kNanosecond);

} // namespace hft::benchmarks
