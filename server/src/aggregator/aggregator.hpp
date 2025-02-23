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
#include "pool/lfq_pool.hpp"
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
      : mSink{sink}, mPriceFeed{mSink, getPricesView()}, mLfqPool(10),
        mLoadBalanceTimer{mSink.ctx()} {
    mSink.dataSink.setConsumer(this);
    mSink.controlSink.addCommandHandler({ServerCommand::CollectStats},
                                        [this](ServerCommand command) {
                                          if (command == ServerCommand::CollectStats) {
                                            getTrafficStats();
                                          }
                                        });
    // This rerouting approach is quite bad: Rtt for rerouted orders min:19ms max:561ms avg:119.0ms
    // For rerouted orders we need to directly move them from old worker queue to a new worker queue
    // scheduleLoadBalancing();
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
        // Steal the current queue to avoid hanging buffers, if its currently in use we will just
        // pop all elements, if not, the previous thread will get a brand new queue from the pool
        auto activeLfq = std::atomic_exchange(&data.rerouteQueue, nullptr);
        if (activeLfq != nullptr && !activeLfq->empty()) {
          spdlog::info("Processing buffer after rerouting");
          std::vector<Order> reroutedOrders;
          Order order;
          while (activeLfq->pop(order)) {
            reroutedOrders.emplace_back(std::move(order));
          }
          data.orderBook->add(Span<Order>(reroutedOrders));
        }
        auto matches = data.orderBook->match();
        if (!matches.empty()) {
          mSink.ioSink.post(Span<OrderStatus>(matches));
        }
      } else {
        std::stringstream ss;
        for (auto &order : subSpan) {
          spdlog::info("Rerouting {} to a buffer", order.id);
        }
        // Either previous thread revisits the rerouted order book cause it still
        // has some orders in its queue, or new thread cannot yet take over cause previous
        // thread was caught for reorder in the midst of operation. Cache the orders
        LFQPool<Order>::SPtrLFQueueType existingLfq = nullptr;
        auto activeLfq = mLfqPool.getLFQueue();
        if (!std::atomic_compare_exchange_strong(&data.rerouteQueue, &existingLfq, activeLfq)) {
          activeLfq = existingLfq;
        }
        for (auto &order : subSpan) {
          while (!activeLfq->push(order)) {
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
    mLoadBalanceTimer.expires_after(MilliSeconds(1)); // Milliseconds(1)
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

    auto rerouted = iter->second.rerouteQueue.load();
    if (rerouted != nullptr) {
      return;
    }

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
  LFQPool<Order> mLfqPool;

  SteadyTimer mLoadBalanceTimer;
};

AggregatorData Aggregator::skData;

} // namespace hft::server

#endif // HFT_SERVER_AGGREGATOR_HPP
