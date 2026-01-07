/**
 * @author Vladimir Pavliv
 * @date 2025-12-30
 */

#ifndef HFT_BENCH_TESTDATA_HPP
#define HFT_BENCH_TESTDATA_HPP

#include "container_types.hpp"
#include "execution/market_data.hpp"
#include "primitive_types.hpp"
#include "ticker.hpp"
#include "utils/id_utils.hpp"
#include "utils/test_utils.hpp"

namespace hft::benchmarks {

struct TestTickerData {
  explicit TestTickerData(size_t count = 0) { generate(count); }

  void generate(size_t count) {
    tickers.clear();
    tickers.reserve(count);
    for (size_t i = 0; i < count; ++i) {
      tickers.emplace_back(utils::generateTicker());
    }
  }
  Vector<Ticker> tickers;
};

struct TestOrderData {
  explicit TestOrderData(TestTickerData &tickersData, size_t orderCount = 0)
      : tickers{tickersData} {
    if (orderCount != 0) {
      generate(orderCount);
    }
  }

  void generate(size_t orderCount) {
    orders.clear();
    orders.reserve(orderCount);

    auto dataIt = tickers.tickers.begin();
    for (size_t i = 0; i < orderCount; ++i) {
      if (dataIt == tickers.tickers.end()) {
        dataIt = tickers.tickers.begin();
      }
      auto order = utils::generateOrder(*dataIt++);
      order.id = i;
      orders.push_back(server::ServerOrder{0, order});
    }
  }

  TestTickerData &tickers;
  Vector<server::ServerOrder> orders;
};

struct TestMarketData {
  explicit TestMarketData(TestTickerData &tickersData, size_t workerCount = 0)
      : tickers{tickersData} {
    if (workerCount != 0) {
      generate(workerCount);
    }
  }

  void generate(size_t workerCount) {
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

  TestTickerData &tickers;
  server::MarketData marketData;
};

} // namespace hft::benchmarks

#endif // HFT_COMMON_TESTMARKETDATA_HPP