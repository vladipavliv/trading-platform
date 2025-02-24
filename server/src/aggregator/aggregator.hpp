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
 * @brief Handles OrderBooks, distributes tickers between threads and performs load balancing
 * Current load balancing approach does not perform very well though, for orders that get caught in
 * some threads queue in the midst of rerouting rtt spikes at least up to 3ms, from 30us
 * Current reroute approach is the following: OrderBook gets locked so no thread works on it right
 * now, and if thread finds orders in its queue that should no more be handled by it - they are just
 * get posted in the sink again. Previous attempt with caching orders right in the order book so new
 * thread could pick them faster, worked even worse. Need more efficient orders injecting mechanism
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
    // scheduleLoadBalancing();
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
      Lock<OrderBook> lock{data.orderBook};
      if (!lock.success || data.getThreadId() != threadId) {
        for (auto &order : subSpan) {
          spdlog::info("Rerouting {}", order.id);
        }
        mSink.dataSink.post(subSpan);
      } else {
        data.orderBook.add(subSpan);
        auto matches = data.orderBook.match();
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

  void balanceLoad() { // Just a randomizer for testing
    auto workers = mSink.dataSink.workers();
    if (workers < 2) {
      return;
    }
    auto iter = skData.begin();
    std::advance(iter, utils::RNG::rng<size_t>(skData.size() - 1));

    Lock<OrderBook> lock{iter->second.orderBook};
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
