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
#include "server_event.hpp"
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
      : bus_{bus}, data_{data}, timer_{bus_.ioCtx()}, statsRate_{Config::cfg.monitorRate} {
    bus_.marketBus.setHandler<Order>([this](Span<Order> orders) { processOrders(orders); });
  }

  void start() {
    startWorkers();
    scheduleStatsTimer();
    bus_.systemBus.publish(ServerEvent::Ready);
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

  void processOrders(Span<Order> orders) {
    // TODO() Try sorting by the worker id and processing in chunks
    ordersTotal_.fetch_add(orders.size(), std::memory_order_relaxed);
    for (auto &order : orders) {
      processOrder(order);
    }
  }

  void processOrder(CRef<Order> order) {
    const auto &data = data_.at(order.ticker);
    workers_[data->getThreadId()]->ioCtx.post([this, order, &data]() {
      data->orderBook.add(order);
      auto matches = data->orderBook.match();
      if (!matches.empty()) {
        bus_.marketBus.publish(Span<OrderStatus>(matches));
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
      static uint64_t lastTtl = 0;
      uint64_t currentTtl = ordersTotal_.load(std::memory_order_relaxed);
      uint64_t rps = (currentTtl - lastTtl) / statsRate_.count();

      auto rpsStr = utils::thousandify(rps);
      auto opnStr = utils::thousandify(countOpenedOrders());
      auto ttlStr = utils::thousandify(currentTtl);
      if (rps != 0) {
        Logger::monitorLogger->info("Orders: [opn|ttl] {}|{} | Rps: {}", opnStr, ttlStr, rpsStr);
      }
      lastTtl = currentTtl;
      scheduleStatsTimer();
    });
  }

  size_t countOpenedOrders() {
    size_t orders = 0;
    for (auto &tickerData : data_) {
      orders += tickerData.second->orderBook.openedOrders();
    }
    return orders;
  }

private:
  ServerBus &bus_;
  const MarketData &data_;

  SteadyTimer timer_;
  Seconds statsRate_;

  std::atomic_uint64_t ordersTotal_;
  std::atomic_uint64_t ordersOpened_;

  std::vector<Worker::UPtr> workers_;
};

} // namespace hft::server

#endif // HFT_SERVER_COORDINATOR_HPP
