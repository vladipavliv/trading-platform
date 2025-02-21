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
#include "comparators.hpp"
#include "config/config.hpp"
#include "db/postgres_adapter.hpp"
#include "market_types.hpp"
#include "order_traffic_stats.hpp"
#include "price_feed.hpp"
#include "server_types.hpp"
#include "utils/utils.hpp"

namespace hft::server {

/**
 * @brief Handles OrderBooks, balances tickers between worker threads. Does not provide
 * any synchronozation, instead relies on scheduling worker threads to work with specific
 * tickers synchronously. Amount of tickers would always be higher then amount of worker threads
 * Tickers are also well known, so we can have lock-free mapping Ticker -> ThreadId
 * When rebalancing would be needed, orderbook can be marked untill previous thread finishes
 * processing its messages, and then next thread takes over. Next thread meanwhile works on
 * other ticker orders, and requests for this ticker are still being processed by previous thread
 * Not sure if thats a viable idea or if thats truly lock-free
 */
class Aggregator {
public:
  Aggregator(ServerSink &sink) : mSink{sink}, mPriceFeed{mSink, getPricesView()} {
    mSink.dataSink.setHandler<Order>([this](Span<Order> orders) { processOrders(orders); });
    mSink.controlSink.addCommandHandler({ServerCommand::CollectStats},
                                        [this](ServerCommand command) {
                                          if (command == ServerCommand::CollectStats) {
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
      spdlog::error("Unknown ticker {}", utils::toStrView(order.ticker));
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
      auto &data = skData[item.ticker];
      data.threadId = roundRobin;
      data.currentPrice = item.price;
      data.orderBook = std::make_unique<OrderBook>(
          item.ticker, [this](Span<OrderStatus> statuses) { mSink.ioSink.post(statuses); });
      roundRobin++;
    }
    return data;
  }

  PricesView getPricesView() {
    initMarketData();
    return PricesView{skData};
  }

  void processOrders(Span<Order> orders) {
    auto [subSpan, leftover] = frontSubspan(orders, TickerCmp<Order>{});
    while (!subSpan.empty()) {
      auto order = subSpan.front();
      if (!skData.contains(order.ticker)) {
        spdlog::error("Unknown ticker {}", std::string(order.ticker.begin(), order.ticker.end()));
        continue;
      }
      auto &data = skData[order.ticker];
      data.orderBook->add(subSpan);
      data.eventCounter.fetch_add(1);

      std::tie(subSpan, leftover) = frontSubspan(leftover, TickerCmp<Order>{});
    }
  }

  void getTrafficStats() {
    OrderTrafficStats stats;
    for (auto &item : skData) {
      stats.processedOrders += item.second.eventCounter.load();
      stats.currentOrders += item.second.orderBook->ordersCount();
    }
    mSink.controlSink.onEvent(stats);
  }

  void generateOrders() {
    auto iterator = skData.begin();
    for (int i = 0; i < 1000000; ++i) {
      if (iterator == skData.end()) {
        iterator = skData.begin();
      }
      std::vector<Order> orders;
      orders.reserve(5);
      for (int i = 0; i < 5; ++i) {
        orders.emplace_back(utils::generateOrder(iterator->first));
      }
      processOrders(orders);
      iterator++;
    }
  }

private:
  ServerSink &mSink;
  static AggregatorData skData;
  PriceFeed mPriceFeed;
};

AggregatorData Aggregator::skData;

} // namespace hft::server

#endif // HFT_SERVER_AGGREGATOR_HPP
