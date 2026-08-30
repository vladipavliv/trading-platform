/**
 * @author Vladimir Pavliv
 * @date 2025-12-30
 */

#ifndef HFT_TESTS_DATAGENERATOR_HPP
#define HFT_TESTS_DATAGENERATOR_HPP

#include "adapters/json/json_adapter.hpp"
#include "config/config.hpp"
#include "constants.hpp"
#include "container_types.hpp"
#include "execution/market_data.hpp"
#include "gateway/internal_order.hpp"
#include "primitive_types.hpp"
#include "ticker.hpp"
#include "ticker/ticker_map.hpp"
#include "utils/market_utils.hpp"
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

inline BookOrderId genBookOId() {
  // static uint32_t counter = 0;
  return BookOrderId::make(1, 1);
}

inline Order genOrder(Ticker ticker, Price p = 0) {
  if (p == 0) {
    p = RNG::generate<Price>(10, MAX_TICKS - 10);
  } else {
    p = fluctuateThePrice(p);
  }
  return Order{genId(), ticker, RNG::generate<Quantity>(10, 1000), p,
               RNG::generate<uint8_t>(0, 1) == 0 ? OrderAction::Buy : OrderAction::Sell};
}

inline Order genOrder() { return genOrder(genTicker()); }

inline InternalOrderEvent genInternalOrder() {
  Order o = genOrder();
  return InternalOrderEvent{
      {SystemOrderId{o.id}, genBookOId(), o.quantity, o.price}, nullptr, o.ticker, o.action};
}

struct GenOrderData {
  explicit GenOrderData(const Config &cfg)
      : cfg{cfg}, orderCount{cfg.get<size_t>("test.order_count")} {
    adapters::JsonAdapter adapter{cfg};
    const auto tickerPrices = adapter.readTickers();
    if (!tickerPrices) {
      throw std::runtime_error("Failed to load prices");
    }
    prices.resize(orderCount);
    for (auto &price : tickerPrices.value()) {
      prices[getTickerIndex(price.ticker)] = price.price;
    }
    gen(orderCount);
  }

  void gen(size_t orderCount) {
    orders.clear();
    orders.reserve(orderCount);

    auto tickerIdx = 0;
    for (size_t i = 0; i < orderCount; ++i) {
      auto o = genOrder(getTicker(tickerIdx), prices[i]);
      InternalOrder io{SlotId<>(i), genBookOId(), o.quantity, o.price};
      orders.push_back(InternalOrderEvent{io, nullptr, o.ticker, o.action});

      if (++tickerIdx == TICKER_COUNT) {
        tickerIdx = 0;
      }
    }
  }

  const Config &cfg;
  const size_t orderCount;
  std::vector<Price> prices;
  Vector<InternalOrderEvent> orders;
};

struct GenMarketData {
  explicit GenMarketData(size_t workerCount = 0) {
    if (workerCount != 0) {
      gen(workerCount);
    }
  }

  void gen(size_t workerCount) {
    ThreadId workerId{0};
    for (size_t i = 0; i < TICKER_COUNT; ++i) {
      marketData[i].workerId = workerId;
      if (++workerId == workerCount) {
        workerId = 0;
      }
    }
  }

  void cleanup() {
    for (auto &td : marketData) {
      td.orderBook.clear();
    }
  }

  MarketData marketData;
};

} // namespace hft::tests

#endif // HFT_TESTS_DATAGENERATOR_HPP