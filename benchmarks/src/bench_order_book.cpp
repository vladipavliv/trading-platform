/**
 * @author Vladimir Pavliv
 * @date 2025-08-02
 */

#include <benchmark/benchmark.h>

#include "bus/busable.hpp"
#include "config/server_config.hpp"
#include "container_types.hpp"
#include "domain/server_order_messages.hpp"
#include "execution/orderbook/flat_order_book.hpp"
#include "execution/orderbook/price_level_order_book.hpp"
#include "gateway/internal_order.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"
#include "traits.hpp"
#include "utils/data_generator.hpp"
#include "utils/rng.hpp"
#include "utils/spin_wait.hpp"

namespace hft::benchmarks {

using namespace server;
using namespace utils;
using namespace tests;

class BM_OrderBookFix : public benchmark::Fixture {
public:
  ServerConfig cfg;
  const bool fullLogs{false};

  uint64_t acceptedCounter{0};
  uint64_t partialCounter{0};
  uint64_t fullCounter{0};
  uint64_t cancelledCounter{0};
  uint64_t rejectedCounter{0};
  uint64_t restingCounter{0};
  uint64_t totalProcessed{0};

  inline static Vector<InternalOrderEvent> orders;
  Vector<BookOrderId> activeOrderIds;

  BM_OrderBookFix() : cfg{"bench_server_config.ini"} {}

  template <typename EventType>
  void post(CRef<EventType>) {}

  void SetUp(const ::benchmark::State &) override {
    const size_t ordersCount = 16384;

    orders.reserve(ordersCount);
    for (size_t i = 0; i < ordersCount; ++i) {
      auto io = genInternalOrder();
      orders.emplace_back(io);
    }
  }

  void TearDown(const ::benchmark::State &) override { orders.clear(); }

  void resetCounters() {
    acceptedCounter = 0;
    partialCounter = 0;
    fullCounter = 0;
    cancelledCounter = 0;
    rejectedCounter = 0;
    restingCounter = 0;
    totalProcessed = 0;
    activeOrderIds.clear();
  }

  void doNotOptimize() {
    benchmark::DoNotOptimize(acceptedCounter);
    benchmark::DoNotOptimize(partialCounter);
    benchmark::DoNotOptimize(fullCounter);
    benchmark::DoNotOptimize(cancelledCounter);
    benchmark::DoNotOptimize(rejectedCounter);
    benchmark::DoNotOptimize(totalProcessed);
  }

  void logCounters(benchmark::State &state) {
    state.counters["acc"] = acceptedCounter;
    state.counters["part"] = partialCounter;
    state.counters["full"] = fullCounter;
    state.counters["cancl"] = cancelledCounter;
    state.counters["rej"] = rejectedCounter;
    state.counters["rest"] = restingCounter;
    state.counters["total"] = totalProcessed;
  }
};

template <>
void BM_OrderBookFix::post<InternalOrderStatus>(CRef<InternalOrderStatus> s) {
  switch (s.state) {
  case OrderState::Accepted: {
    ++acceptedCounter;
    ++restingCounter;
    activeOrderIds.push_back(s.bookOId);
    break;
  }
  case OrderState::Partial: {
    ++partialCounter;
    ++restingCounter;
    activeOrderIds.push_back(s.bookOId);
    break;
  }
  case OrderState::Full: {
    ++fullCounter;
    break;
  }
  case OrderState::Cancelled: {
    ++cancelledCounter;
    break;
  }
  case OrderState::Rejected: {
    ++rejectedCounter;
    break;
  }
  default:
    break;
  }
  ++totalProcessed;
}

BENCHMARK_F(BM_OrderBookFix, Latency)(benchmark::State &state) {
  OrderBook book;

  for (size_t i = 0; i < 1000; ++i) {
    book.add(orders[i % orders.size()], *this);
  }
  book.clear();
  resetCounters();

  auto iter = orders.begin();
  for (auto _ : state) {
    if (iter == orders.end()) {
      iter = orders.begin();

      state.PauseTiming();
      book.clear();
      resetCounters();
      state.ResumeTiming();
    }

    benchmark::DoNotOptimize(&*iter);
    book.add(*iter++, *this);
    benchmark::ClobberMemory();
  }
  doNotOptimize();
}

BENCHMARK_F(BM_OrderBookFix, Throughput)(benchmark::State &state) {
  OrderBook book;
  const uint64_t ordersCount = orders.size();

  while (state.KeepRunningBatch(ordersCount)) {
    resetCounters();

    for (const auto &order : orders) {
      benchmark::DoNotOptimize(&order);
      book.add(order, *this);
    }
    benchmark::ClobberMemory();

    state.PauseTiming();
    book.clear();
    state.ResumeTiming();
  }
  doNotOptimize();
}

BENCHMARK_F(BM_OrderBookFix, LatencyCancel)(benchmark::State &state) {
  OrderBook book;

  for (size_t i = 0; i < 1000; ++i) {
    book.add(orders[i % orders.size()], *this);
  }
  book.clear();
  resetCounters();

  auto iter = orders.begin();
  for (auto _ : state) {
    if (iter == orders.end()) {
      iter = orders.begin();

      state.PauseTiming();
      book.clear();
      resetCounters();
      state.ResumeTiming();
    }

    if (!activeOrderIds.empty() && RNG::generate(0, 3) == 0) {
      size_t idx = RNG::generate<size_t>(0, activeOrderIds.size() - 1);
      BookOrderId targetId = activeOrderIds[idx];
      activeOrderIds[idx] = activeOrderIds.back();
      activeOrderIds.pop_back();

      InternalOrderEvent cancelEvent;
      cancelEvent.order = InternalOrder{SystemOrderId{genId()}, targetId, 0, 0};
      cancelEvent.action = OrderAction::Cancel;

      benchmark::DoNotOptimize(&cancelEvent);
      book.add(cancelEvent, *this);
      benchmark::ClobberMemory();
    } else {
      benchmark::DoNotOptimize(&*iter);
      book.add(*iter++, *this);
      benchmark::ClobberMemory();
    }
  }

  if (fullLogs) {
    logCounters(state);
  }
  doNotOptimize();
}

BENCHMARK_F(BM_OrderBookFix, ThroughputCancel)(benchmark::State &state) {
  OrderBook book;
  const uint64_t ordersCount = orders.size();

  uint64_t totalAccepted = 0;
  uint64_t totalPartial = 0;
  uint64_t totalFull = 0;
  uint64_t totalCancelled = 0;
  uint64_t totalRejected = 0;
  uint64_t totalResting = 0;
  uint64_t totalAll = 0;

  while (state.KeepRunningBatch(ordersCount)) {
    resetCounters();

    for (const auto &order : orders) {
      benchmark::DoNotOptimize(&order);

      if (!activeOrderIds.empty() && RNG::generate(0, 3) == 0) {
        size_t idx = RNG::generate<size_t>(0, activeOrderIds.size() - 1);
        BookOrderId targetId = activeOrderIds[idx];
        activeOrderIds[idx] = activeOrderIds.back();
        activeOrderIds.pop_back();

        InternalOrderEvent cancelEvent;
        cancelEvent.order = InternalOrder{SystemOrderId{genId()}, targetId, 0, 0};
        cancelEvent.action = OrderAction::Cancel;

        book.add(cancelEvent, *this);
      } else {
        book.add(order, *this);
      }
    }
    benchmark::ClobberMemory();

    totalAccepted += acceptedCounter;
    totalPartial += partialCounter;
    totalFull += fullCounter;
    totalCancelled += cancelledCounter;
    totalRejected += rejectedCounter;
    totalResting += restingCounter;
    totalAll += totalProcessed;

    state.PauseTiming();
    book.clear();
    state.ResumeTiming();
  }

  if (fullLogs) {
    logCounters(state);
  }

  doNotOptimize();
}

} // namespace hft::benchmarks