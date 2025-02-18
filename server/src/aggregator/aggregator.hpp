/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_AGGREGATOR_HPP
#define HFT_SERVER_AGGREGATOR_HPP

#include <map>
#include <ranges>
#include <spdlog/spdlog.h>

#include "aggregator_data.hpp"
#include "boost_types.hpp"
#include "config/config.hpp"
#include "db/postgres_adapter.hpp"
#include "market_types.hpp"
#include "order_book.hpp"
#include "price_feed.hpp"
#include "server_types.hpp"
#include "utils/utils.hpp"

namespace hft::server {

/**
 * @brief Handles OrderBooks, balances tickers between worker threads
 */
class Aggregator {
public:
  Aggregator(ServerSink &sink) : mSink{sink}, mPriceFeed{mSink, getPricesView()} {
    mSink.dataSink.setHandler<Order>([this](const Order &order) { processOrder(order); });
    mSink.controlSink.addCommandHandler({ServerCommand::GetTrafficStats},
                                        [this](ServerCommand command) {
                                          if (command == ServerCommand::GetTrafficStats) {
                                            getTrafficStats();
                                          }
                                        });
  }

  void start() {}
  void stop() {}

  /**
   * @brief Load balancer, returns the worker thread id to handle requests for ticker
   * @todo Upon balancing would have to carefully let one thread finish processing current
   * ticker events in its queue, while new events incoming to new threads queue. Think it through
   */
  static ThreadId getWorkerId(const Order &order) {
    if (!skData.contains(order.ticker)) {
      spdlog::error("Unknown ticker {}", utils::toString(order.ticker));
      return std::numeric_limits<uint8_t>::max();
    }
    return skData[order.ticker].threadId;
  }

private:
  AggregatorData initMarketData() {
    AggregatorData data;
    auto tickers = db::PostgresAdapter::readTickers();

    ThreadId roundRobin = 0;
    for (auto &item : tickers) {
      if (roundRobin >= Config::cfg.coresApp.size()) {
        roundRobin = 0;
      }
      skData[item.ticker].threadId = roundRobin;
      skData[item.ticker].currentPrice = item.price;
      skData[item.ticker].orderBook = std::make_unique<OrderBook>(
          item.ticker, [this](const OrderStatus &status) { mSink.ioSink.post(status); });
      roundRobin++;
    }
    return data;
  }

  PricesView getPricesView() {
    initMarketData();
    return PricesView{skData};
  }

  void processOrder(const Order &order) {
    spdlog::debug("Processing order {}", utils::toString(order));
    if (!skData.contains(order.ticker)) {
      spdlog::error("Unknown ticker {}", std::string(order.ticker.begin(), order.ticker.end()));
      return;
    }
    auto &data = skData[order.ticker];
    data.orderBook->add(order);
    data.eventCounter.fetch_add(1, std::memory_order_relaxed);
  }

  void getTrafficStats() {
    TrafficStats stats;
    for (auto &item : skData) {
      stats.processedOrders += item.second.eventCounter.load();
      stats.currentOrders += item.second.orderBook->ordersCount();
    }
    mSink.controlSink.onEvent(stats);
  }

private:
  ServerSink &mSink;
  static AggregatorData skData;
  PriceFeed mPriceFeed;
};

AggregatorData Aggregator::skData;

} // namespace hft::server

#endif // HFT_SERVER_AGGREGATOR_HPP
