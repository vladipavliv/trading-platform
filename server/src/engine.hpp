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
#include "price_feed.hpp"
#include "ticker_data.hpp"
#include "worker.hpp"

namespace hft::server {

class Engine {
public:
  Engine(IoContext &ctx)
      : data_{readMarketData()}, priceFeed_{data_, ctx}, statsTimer_{ctx},
        statsRate_{Seconds{Config::cfg.monitorRateS}} {
    EventBus::bus().subscribe<Order>([this](Span<Order> orders) { processOrders(orders); });
    scheduleStatsTimer();
    startWorkers();
  }

  void stop() {
    for (auto &worker : workers_) {
      worker->stop();
    }
  }

private:
  MarketData readMarketData() {
    MarketData data;
    auto prices = db::PostgresAdapter::readTickers();
    ThreadId roundRobin = 0;
    for (auto &item : prices) {
      data.emplace(item.ticker, std::make_unique<TickerData>(roundRobin, item.price));
      roundRobin = (++roundRobin < Config::cfg.coreIds.size()) ? roundRobin : 0;
    }
    Logger::monitorLogger->info(std::format("Market data loaded for {} tickers", data_.size()));
    return data;
  }

  void startWorkers() {
    workers_.reserve(Config::cfg.coreIds.size());
    for (int i = 0; i < Config::cfg.coreIds.size(); ++i) {
      workers_.emplace_back(std::make_unique<Worker>(i));
    }
  }

  void processOrders(Span<Order> orders) {
    ordersTotal_.fetch_add(1, std::memory_order_relaxed);
    // TODO() process in chunks
    for (auto &order : orders) {
      auto &data = data_[order.ticker];
      workers_[data->getThreadId()]->post([this, order, &data]() {
        data->orderBook.add(order);
        auto matches = data->orderBook.match();
        if (!matches.empty()) {
          EventBus::bus().publish(Span<OrderStatus>(matches));
        }
        ordersClosed_.fetch_add(matches.size(), std::memory_order_relaxed);
      });
    }
  }

  void scheduleStatsTimer() {
    statsTimer_.expires_after(statsRate_);
    statsTimer_.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        return;
      }
      static size_t lastOrderCount = 0;
      size_t ordersCurrent = ordersTotal_.load(std::memory_order_relaxed);
      auto rps = (ordersCurrent - lastOrderCount) / statsRate_.count();
      if (rps != 0) {
        Logger::monitorLogger->info("Orders [matched|total] {} {} rps:{}",
                                    ordersClosed_.load(std::memory_order_relaxed),
                                    ordersTotal_.load(std::memory_order_relaxed), rps);
      }
      lastOrderCount = ordersCurrent;
      scheduleStatsTimer();
    });
  }

private:
  std::vector<Worker::UPtr> workers_;
  MarketData data_;
  PriceFeed priceFeed_;

  SteadyTimer statsTimer_;
  Seconds statsRate_;

  std::atomic_size_t ordersTotal_;
  std::atomic_size_t ordersClosed_;
};

} // namespace hft::server

#endif // HFT_SERVER_ENGINE_HPP