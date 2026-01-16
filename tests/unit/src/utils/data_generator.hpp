/**
 * @author Vladimir Pavliv
 * @date 2025-12-30
 */

#ifndef HFT_TESTS_DATAGENERATOR_HPP
#define HFT_TESTS_DATAGENERATOR_HPP

#include "container_types.hpp"
#include "execution/market_data.hpp"
#include "gateway/internal_order.hpp"
#include "primitive_types.hpp"
#include "ticker.hpp"
#include "utils/rng.hpp"
#include "utils/time_utils.hpp"

namespace hft::tests {

using namespace server;
using namespace utils;

inline auto genId() -> uint32_t {
  static uint64_t counter{0};
  return counter++;
}

inline Ticker genTicker() {
  Ticker result;
  for (size_t i = 0; i < 4; ++i) {
    result[i] = RNG::generate<uint8_t>((uint8_t)'A', (uint8_t)'Z');
  }
  return result;
}

inline Order genOrder(Ticker ticker = {'G', 'O', 'O', 'G'}) {
  return Order{genId(),
               getCycles(),
               ticker,
               RNG::generate<Quantity>(0, 1000),
               RNG::generate<Price>(10, 10000),
               RNG::generate<uint8_t>(0, 1) == 0 ? OrderAction::Buy : OrderAction::Sell};
}

inline InternalOrderEvent genInternalOrder() {
  Order o = genOrder();
  return InternalOrderEvent{
      {SystemOrderId{o.id}, o.quantity, o.price}, nullptr, o.ticker, o.action};
}

struct GenTickerData {
  explicit GenTickerData(size_t count = 0) { gen(count); }

  void gen(size_t count) {
    tickers.clear();
    tickers.reserve(count);
    for (size_t i = 0; i < count; ++i) {
      tickers.emplace_back(genTicker());
    }
  }
  Vector<Ticker> tickers;
};

struct GenOrderData {
  explicit GenOrderData(GenTickerData &tickersData, size_t orderCount = 0) : tickers{tickersData} {
    if (orderCount != 0) {
      gen(orderCount);
    }
  }

  void gen(size_t orderCount) {
    orders.clear();
    orders.reserve(orderCount);

    auto dataIt = tickers.tickers.begin();
    for (size_t i = 0; i < orderCount; ++i) {
      if (dataIt == tickers.tickers.end()) {
        dataIt = tickers.tickers.begin();
      }
      auto o = genOrder(*dataIt++);
      InternalOrder io{SlotId<>(i), o.quantity, o.price};
      orders.push_back(InternalOrderEvent{io, nullptr, o.ticker, o.action});
    }
  }

  GenTickerData &tickers;
  Vector<InternalOrderEvent> orders;
};

struct GenMarketData {
  explicit GenMarketData(GenTickerData &tickersData, size_t workerCount = 0)
      : tickers{tickersData} {
    if (workerCount != 0) {
      gen(workerCount);
    }
  }

  void gen(size_t workerCount) {
    marketData.clear();
    marketData.reserve(tickers.tickers.size());

    ThreadId workerId{0};
    for (auto &ticker : tickers.tickers) {
      marketData.emplace(ticker, workerId);
      if (++workerId == workerCount) {
        workerId = 0;
      }
    }
  }

  void cleanup() {
    for (auto &td : marketData) {
      td.second.orderBook.clear();
    }
  }

  GenTickerData &tickers;
  MarketData marketData;
};

} // namespace hft::tests

#endif // HFT_TESTS_DATAGENERATOR_HPP