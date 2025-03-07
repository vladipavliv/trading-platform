/**
 * @author Vladimir Pavliv
 * @date 2025-02-20
 */

#ifndef HFT_SERVER_COORDINATOR_HPP
#define HFT_SERVER_COORDINATOR_HPP

#include <map>
#include <vector>

#include "market_types.hpp"
#include "order_book.hpp"
#include "server_bus.hpp"
#include "server_events.hpp"
#include "ticker_data.hpp"
#include "worker.hpp"

namespace hft::server {

/**
 * @brief Subscribes to Orders and coordinates them between workers
 * Later on would do load balancing and adding/removing workers dynamically
 */
class Coordinator {
public:
  Coordinator(ServerBus &bus, const MarketData &data)
      : bus_{bus}, data_{data}, timer_{bus.systemBus.systemIoCtx},
        statsRate_{Config::cfg.monitorRate} {
    bus_.marketBus.setHandler<Order>([this](Span<Order> orders) { processOrders(orders); });
  }

  void start() {
    startWorkers();
    if (Config::cfg.coresWarmup.count() != 0) {
      warmUpWorkers();
    } else {
      workersReady();
    }
  }

  void stop() {
    for (auto &worker : workers_) {
      worker->ioCtx.stop();
    }
  }

private:
  void startWorkers() {
    const auto appCores = Config::cfg.coresApp.size();
    Logger::monitorLogger->info("Starting {} workers", appCores);
    workers_.reserve(appCores);
    for (int i = 0; i < appCores; ++i) {
      workers_.emplace_back(std::make_unique<Worker>(i));
    }
  }

  void warmUpWorkers() {
    Logger::monitorLogger->info("Warming up workers");
    for (auto &worker : workers_) {
      worker->warmUpStart();
    }
    timer_.expires_after(Config::cfg.coresWarmup);
    timer_.async_wait([this](BoostErrorRef ec) {
      for (auto &worker : workers_) {
        worker->warmUpStop();
      }
      workersReady();
    });
  }

  void workersReady() {
    bus_.systemBus.publish(ServerEvent::CoresWarmedUp);
    scheduleStatsTimer();
  }

  void processOrders(Span<Order> orders) {
    // TODO() Try sorting by the worker id and processing in chunks
    for (auto &order : orders) {
      processOrder(order);
    }
  }

  void processOrder(CRef<Order> order) {
    ordersTotal_.fetch_add(1, std::memory_order_relaxed);
    const auto &data = data_.at(order.ticker);
    workers_[data->getThreadId()]->ioCtx.post([this, order, &data]() {
      data->orderBook.add(order);
      auto matches = data->orderBook.match();
      if (!matches.empty()) {
        bus_.marketBus.publish(Span<OrderStatus>(matches));
        ordersMatched_.fetch_add(matches.size(), std::memory_order_relaxed);
      }
    });
  }

  void scheduleStatsTimer() {
    timer_.expires_after(statsRate_);
    timer_.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        Logger::monitorLogger->error("Error {}", ec.message());
        return;
      }
      static size_t lastOrderCount = 0;
      size_t ordersCurrent = ordersTotal_.load(std::memory_order_relaxed);
      auto rps = (ordersCurrent - lastOrderCount) / statsRate_.count();
      if (rps != 0) {
        Logger::monitorLogger->info("Orders [matched|total] {} {} rps:{}",
                                    ordersMatched_.load(std::memory_order_relaxed),
                                    ordersTotal_.load(std::memory_order_relaxed), rps);
      }
      lastOrderCount = ordersCurrent;
      scheduleStatsTimer();
    });
  }

private:
  ServerBus &bus_;
  const MarketData &data_;

  SteadyTimer timer_;
  Seconds statsRate_;

  std::atomic_size_t ordersTotal_;
  std::atomic_size_t ordersMatched_;

  std::vector<Worker::UPtr> workers_;
};

} // namespace hft::server

#endif // HFT_SERVER_COORDINATOR_HPP