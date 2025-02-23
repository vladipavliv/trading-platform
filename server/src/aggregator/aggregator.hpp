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

#include "aggregator_data.hpp"
#include "boost_types.hpp"
#include "comparators.hpp"
#include "config/config.hpp"
#include "db/postgres_adapter.hpp"
#include "locker.hpp"
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
    scheduleLoadBalancing();
  }

  void onEvent(ThreadId threadId, Span<Order> orders) {
    auto [subSpan, leftover] = frontSubspan(orders, TickerCmp<Order>{});
    while (!subSpan.empty()) {
      auto order = subSpan.front();
      if (!skData.contains(order.ticker)) {
        spdlog::error("Unknown ticker {}", [&order] { return utils::toStrView(order.ticker); }());
        break;
      }
      auto &data = skData[order.ticker];
      Lock<OrderBook> lock{*data.orderBook};
      if (!lock.success || data.getThreadId() != threadId) {
        // When rerouting happens OrderBook gets locked first and then ThreadId gets switched
        // So Thread wont get caught in the middle of the work with the book.
        // So if we fail to lock the book here that means other thread took over.
        // Push the orders back to the sink so they get handled by the new thread
        for (auto &order : subSpan) {
          spdlog::info("Rerouting {}", order.id);
        }
        mSink.dataSink.post(subSpan);
      } else {
        data.orderBook->add(subSpan);
        auto matches = data.orderBook->match();
        if (!matches.empty()) {
          for (auto &status : matches) {
            if (status.state == OrderState::Full) {
              mOrdersClosed.fetch_add(1, std::memory_order_relaxed);
            }
          }
          mSink.ioSink.post(Span<OrderStatus>(matches));
        }
      }
      mOrdersTotal.fetch_add(subSpan.size(), std::memory_order_relaxed);
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
    stats.ordersTotal = mOrdersTotal.load(std::memory_order_relaxed);
    stats.ordersClosed = mOrdersClosed.load(std::memory_order_relaxed);
    mSink.controlSink.onEvent(stats);
  }

  void scheduleLoadBalancing() {
    mLoadBalanceTimer.expires_after(MilliSeconds(1));
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

    Lock<OrderBook> lock{*(iter->second.orderBook)};
    if (!lock.success) {
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

  std::atomic_size_t mOrdersTotal;
  std::atomic_size_t mOrdersClosed;

  SteadyTimer mLoadBalanceTimer;
};

AggregatorData Aggregator::skData;

} // namespace hft::server

#endif // HFT_SERVER_AGGREGATOR_HPP
