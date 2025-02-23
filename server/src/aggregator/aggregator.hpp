/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_AGGREGATOR_HPP
#define HFT_SERVER_AGGREGATOR_HPP

#include <fmt/core.h>
#include <map>
#include <ranges>
#include <spdlog/spdlog.h>

#include "acquirer.hpp"
#include "aggregator_data.hpp"
#include "boost_types.hpp"
#include "comparators.hpp"
#include "config/config.hpp"
#include "db/postgres_adapter.hpp"
#include "market_types.hpp"
#include "order_traffic_stats.hpp"
#include "price_feed.hpp"
#include "result.hpp"
#include "server_types.hpp"
#include "utils/utils.hpp"

namespace hft::server {

/**
 * @brief Handles OrderBooks, balances tickers between worker threads. Does not provide
 * any synchronozation, instead relies on scheduling worker threads to work with specific
 * tickers synchronously. Amount of tickers would always be higher then amount of worker threads
 * Tickers are also well known, so we can have lock-free mapping Ticker -> ThreadId
 * When rebalancing would be needed, orderbook can be marked untill previous thread finishes
 * processing its messages, and then next thread takes over. Next thread meanwhile works on other
 * ticker orders, and previous requests for this ticker are still being processed by previous thread
 * Not sure if thats a viable idea or if thats truly lock-free stuff
 */
class Aggregator {
public:
  Aggregator(ServerSink &sink)
      : mSink{sink}, mPriceFeed{mSink, getPricesView()}, mLoadBalanceTimer{mSink.ctx()} {
    mSink.dataSink.setConsumer(this);
    mSink.controlSink.addCommandHandler({ServerCommand::CollectStats},
                                        [this](ServerCommand command) {
                                          if (command == ServerCommand::CollectStats) {
                                            getTrafficStats();
                                          }
                                        });
    scheduleLoadBalancing();
  }

  void onEvent(ThreadId threadId, Span<Order> orders) {
    auto [subSpan, leftover] = frontSubspan(orders, TickerCmp<Order>{});
    while (!subSpan.empty()) {
      auto order = subSpan.front();
      if (!skData.contains(order.ticker)) {
        spdlog::error("Unknown ticker {}", [&order] { return utils::toStrView(order.ticker); }());
        continue;
      }
      auto &data = skData[order.ticker];
      AcquiRer<OrderBook> ack(*data.orderBook);
      if (data.getThreadId() == threadId && ack.success) {
        data.orderBook->add(subSpan);
        if (!data.rerouteQueue.empty()) {
          spdlog::info("Processing buffer after rerouting");
          std::vector<Order> rerouted;
          Order order;
          while (data.rerouteQueue.pop(order)) {
            rerouted.emplace_back(std::move(order));
          }
          data.orderBook->add(Span<Order>(rerouted));
        }
        auto matches = data.orderBook->match();
        data.orderBook->release();
        if (!matches.empty()) {
          mSink.ioSink.post(Span<OrderStatus>(matches));
        }
      } else {
        spdlog::info("Rerouting {} orders to a buffer", subSpan.size());
        // Either previous thread revisits the rerouted order book cause it still
        // has some orders in its queue, or new thread cannot yet take over cause previous
        // thread was caught for reorder in the midst of operation. Cache the orders
        for (auto &order : subSpan) {
          while (!data.rerouteQueue.push(order)) {
            std::this_thread::yield();
          }
        }
      }
      data.eventCounter.fetch_add(1);
      std::tie(subSpan, leftover) = frontSubspan(leftover, TickerCmp<Order>{});
    }
  }

  ThreadId getWorkerId(const Order &order) {
    assert(skData.contains(order.ticker));
    return skData[order.ticker].getThreadId();
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
      data.setThreadId(roundRobin);
      data.currentPrice = item.price;
      roundRobin++;
    }
    return data;
  }

  PricesView getPricesView() {
    initMarketData();
    return PricesView{skData};
  }

  void getTrafficStats() {
    OrderTrafficStats stats;
    for (auto &item : skData) {
      stats.processedOrders += item.second.eventCounter.load();
      stats.currentOrders += item.second.orderBook->ordersCount();
    }
    mSink.controlSink.onEvent(stats);
  }

  void scheduleLoadBalancing() {
    mLoadBalanceTimer.expires_after(Seconds(1)); // Milliseconds(1)
    mLoadBalanceTimer.async_wait([this](BoostErrorRef ec) {
      if (!ec) {
        balanceLoad();
        scheduleLoadBalancing();
      }
    });
  }

  void balanceLoad() {
    auto workers = mSink.dataSink.workers();
    if (workers < 2) {
      return;
    }
    auto iter = skData.begin();
    std::advance(iter, utils::RNG::rng<size_t>(skData.size() - 1));

    ThreadId prevWorker = iter->second.getThreadId();
    ThreadId newWorker = (prevWorker == workers - 1) ? 0 : prevWorker + 1;
    iter->second.setThreadId(newWorker);
    spdlog::info(
        "Ticker {} have been rerouted from {} to {} ",
        [&iter] { return utils::toStrView(iter->first); }(), prevWorker, newWorker);
  }

private:
  ServerSink &mSink;
  static AggregatorData skData;
  PriceFeed mPriceFeed;

  SteadyTimer mLoadBalanceTimer;
};

AggregatorData Aggregator::skData;

} // namespace hft::server

#endif // HFT_SERVER_AGGREGATOR_HPP
