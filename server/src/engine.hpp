/**
 * @author Vladimir Pavliv
 * @date 2025-02-20
 */

#ifndef HFT_SERVER_ENGINE_HPP
#define HFT_SERVER_ENGINE_HPP

#include <map>
#include <vector>

#include "db/postgres_adapter.hpp"
#include "event_bus.hpp"
#include "market_types.hpp"
#include "order_book.hpp"
#include "ticker_data.hpp"
#include "worker.hpp"

namespace hft::server {

class Engine {
public:
  Engine(IoContext &ctx) : mStatsTimer{ctx}, mStatsRate{Seconds{Config::cfg.monitorRateS}} {
    EventBus::bus().subscribe<Order>([this](Span<Order> orders) { processOrders(orders); });
    initMarketData();
    scheduleStatsTimer();
    startWorkers();
  }

  void stop() {
    for (auto &worker : mWorkers) {
      worker->stop();
    }
  }

private:
  void initMarketData() {
    auto prices = db::PostgresAdapter::readTickers();
    ThreadId roundRobin = 0;
    for (auto &item : prices) {
      mData.emplace(item.ticker, std::make_unique<TickerData>(roundRobin, item.price));
      if (++roundRobin >= Config::cfg.coreIds.size()) {
        roundRobin = 0;
      }
    }
    Logger::monitorLogger->info(std::format("Market data loaded for {} tickers", mData.size()));
  }

  void startWorkers() {
    mWorkers.reserve(Config::cfg.coreIds.size());
    for (int i = 0; i < Config::cfg.coreIds.size(); ++i) {
      mWorkers.emplace_back(std::make_unique<Worker>(i));
    }
  }

  void processOrders(Span<Order> orders) {
    mOrdersTotal.fetch_add(1, std::memory_order_relaxed);
    // TODO() process in chunks
    for (auto &order : orders) {
      auto &data = mData[order.ticker];
      mWorkers[data->getThreadId()]->post([this, order, &data]() {
        data->orderBook.add(order);
        auto matches = data->orderBook.match();
        if (!matches.empty()) {
          EventBus::bus().publish(Span<OrderStatus>(matches));
        }
        mOrdersClosed.fetch_add(matches.size(), std::memory_order_relaxed);
      });
    }
  }

  void scheduleStatsTimer() {
    mStatsTimer.expires_after(mStatsRate);
    mStatsTimer.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        return;
      }
      static size_t lastOrderCount = 0;
      size_t ordersCurrent = mOrdersTotal.load(std::memory_order_relaxed);
      auto rps = (ordersCurrent - lastOrderCount) / mStatsRate.count();
      if (rps != 0) {
        Logger::monitorLogger->info("Orders [matched|total] {} {} rps:{}",
                                    mOrdersClosed.load(std::memory_order_relaxed),
                                    mOrdersTotal.load(std::memory_order_relaxed), rps);
      }
      lastOrderCount = ordersCurrent;
      scheduleStatsTimer();
    });
  }

private:
  std::vector<Worker::UPtr> mWorkers;
  MarketData mData;

  SteadyTimer mStatsTimer;
  Seconds mStatsRate;

  std::atomic_size_t mOrdersTotal;
  std::atomic_size_t mOrdersClosed;
};

} // namespace hft::server

#endif // HFT_SERVER_ENGINE_HPP