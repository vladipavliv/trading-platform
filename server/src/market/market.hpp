/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_MARKET_HPP
#define HFT_SERVER_MARKET_HPP

#include <map>
#include <ranges>
#include <spdlog/spdlog.h>

#include "boost_types.hpp"
#include "config/config.hpp"
#include "db/postgres_adapter.hpp"
#include "market_data.hpp"
#include "market_types.hpp"
#include "order_book.hpp"
#include "price_feed.hpp"
#include "server_types.hpp"
#include "utils/utils.hpp"

namespace hft::server {

class Market {
public:
  Market(ServerSink &sink) : mSink{sink}, mPriceFeed{mSink, getPricesView()}, mTimer{mSink.ctx()} {
    mSink.dataSink.setHandler<Order>([this](const Order &order) { processOrder(order); });
    mSink.controlSink.setHandler([this](ServerCommand command) {
      switch (command) {
      case ServerCommand::PriceFeedStart:
        mPriceFeed.start();
        break;
      case ServerCommand::PriceFeedStop:
        mPriceFeed.stop();
        break;
      case ServerCommand::MarketStatsShow:
        showStats(true);
        break;
      case ServerCommand::MarketStatsHide:
        showStats(false);
        break;
      default:
        break;
      }
    });
  }

  void start() {}
  void stop() {}

  /**
   * @brief Load balancer
   */
  static ThreadId getWorkerId(const Order &order) {
    if (!skMarketData.contains(order.ticker)) {
      spdlog::error("Unknown ticker {}", utils::toString(order.ticker));
      return std::numeric_limits<uint8_t>::max();
    }
    return skMarketData[order.ticker].threadId;
  }

private:
  MarketData initMarketData() {
    MarketData data;
    auto tickers = db::PostgresAdapter::readTickers();

    ThreadId roundRobin = 0;
    for (auto &item : tickers) {
      if (roundRobin >= Config::cfg.coresApp.size()) {
        roundRobin = 0;
      }
      skMarketData[item.ticker].threadId = roundRobin;
      skMarketData[item.ticker].currentPrice = item.price;
      skMarketData[item.ticker].orderBook = std::make_unique<OrderBook>(
          item.ticker, [this](const OrderStatus &status) { mSink.networkSink.post(status); });
      roundRobin++;
    }
    return data;
  }

  PricesView getPricesView() {
    initMarketData();
    return PricesView{skMarketData};
  }

  void processOrder(const Order &order) {
    // spdlog::debug("Processing order {}", utils::toString(order));
    if (!skMarketData.contains(order.ticker)) {
      spdlog::error("Unknown ticker {}", std::string(order.ticker.begin(), order.ticker.end()));
      return;
    }
    auto &data = skMarketData[order.ticker];
    data.orderBook->add(order);
    data.eventCounter.fetch_add(1, std::memory_order_relaxed);
  }

  void showStats(bool showStats) {
    mShowStats = showStats;
    if (mShowStats) {
      doPrintStats();
    } else {
      mStats.ordersCurrent = 0;
      mStats.requestsProcessed = 0;
      mTimer.cancel();
    }
  }

  void doPrintStats() {
    if (!mShowStats) {
      return;
    }
    size_t requestsCurrent{0};
    size_t ordersCurrent{0};
    for (auto &item : skMarketData) {
      requestsCurrent += item.second.eventCounter.load();
      ordersCurrent += item.second.orderBook->ordersCount();
    }
    std::string rps;
    if (mStats.requestsProcessed != 0) {
      rps = std::format("RPS: {}", (requestsCurrent - mStats.requestsProcessed) / 10);
    }
    mStats.requestsProcessed = requestsCurrent;
    mStats.ordersCurrent = ordersCurrent;
    spdlog::info("Orders total: {} current: {} {}", mStats.requestsProcessed, mStats.ordersCurrent,
                 rps);

    mTimer.expires_after(Seconds(mStatsRate));
    mTimer.async_wait([this](BoostErrorRef ec) { doPrintStats(); });
  }

private:
  ServerSink &mSink;

  static MarketData skMarketData;
  PriceFeed mPriceFeed;

  uint8_t mStatsRate{10};
  std::atomic_bool mShowStats{true};
  MarketStats mStats;
  SteadyTimer mTimer;
};

MarketData Market::skMarketData;

} // namespace hft::server

#endif // HFT_SERVER_MARKET_HPP
