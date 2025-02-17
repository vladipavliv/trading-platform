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
  Market(ServerSink &sink) : mSink{sink}, mPriceFeed{mSink, getPricesView()} {
    mSink.dataSink.setHandler<Order>([this](const Order &order) { processOrder(order); });
    mSink.controlSink.setHandler(ServerCommand::PriceFeedStart,
                                 [this](ServerCommand command) { mPriceFeed.start(); });
    mSink.controlSink.setHandler(ServerCommand::PriceFeedStop,
                                 [this](ServerCommand command) { mPriceFeed.stop(); });
    mSink.controlSink.setHandler(ServerCommand::PrintMarketData,
                                 [this](ServerCommand command) { printData(); });
    mSink.controlSink.setHandler(ServerCommand::ShowMarketStats,
                                 [this](ServerCommand command) { printStats(); });
  }

  void start() {}
  void stop() {}

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

  void printData() {
    std::stringstream ss;
    spdlog::info("Market contains {} tickers:", skMarketData.size());
    auto roundRobin = 0;
    for (auto &item : skMarketData) {
      ss << std::string(item.first.begin(), item.first.end()) << " $"
         << item.second.currentPrice.load() << " ";
      if (++roundRobin == 10) {
        roundRobin = 0;
        spdlog::info(ss.str());
        ss.str("");
        ss.clear();
      }
    }
    spdlog::info(ss.str());
  }

  void printStats() {
    size_t currentOrders = 0;
    std::vector<size_t> eventsProcessed;
    eventsProcessed.resize(Config::cfg.coresApp.size());
    std::map<Ticker, size_t> tickerStats;
    for (auto &item : skMarketData) {
      eventsProcessed[item.second.threadId.load()] += item.second.eventCounter.load();
      currentOrders += item.second.orderBook->ordersCount();
      if (item.second.eventCounter > 0) {
        tickerStats[item.first]++;
      }
    }
    spdlog::info("Market stats:");
    spdlog::info("Current orders: {}", currentOrders);

    ThreadId id{0};
    for (auto &item : eventsProcessed) {
      spdlog::debug("Thread {} processed {} orders", id++, item);
    }
    return;
    int roundRobin = 0;
    std::string tickerStatStr;
    for (auto &item : tickerStats) {
      tickerStatStr += std::format("{}: {}", item.first.data(), item.second);
      if (roundRobin++ > 10) {
        spdlog::info(tickerStatStr);
        tickerStatStr.clear();
        roundRobin = 0;
      } else {
        tickerStatStr += ',';
      }
    }
    if (!tickerStatStr.empty()) {
      spdlog::info(tickerStatStr);
    }
  }

private:
  ServerSink &mSink;

  static MarketData skMarketData;
  PriceFeed mPriceFeed;
};

MarketData Market::skMarketData;

} // namespace hft::server

#endif // HFT_SERVER_MARKET_HPP
