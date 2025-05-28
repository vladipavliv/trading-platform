/**
 * @author Vladimir Pavliv
 * @date 2025-02-20
 */

#ifndef HFT_SERVER_COORDINATOR_HPP
#define HFT_SERVER_COORDINATOR_HPP

#include <vector>

#include "config/server_config.hpp"
#include "ctx_runner.hpp"
#include "domain_types.hpp"
#include "order_book.hpp"
#include "server_events.hpp"
#include "server_ticker_data.hpp"
#include "server_types.hpp"

namespace hft::server {

/**
 * @brief Redirects Orders to a proper worker
 * @todo Later on would perform ticker rerouting and dynamic worker add/remove
 */
class Coordinator {
public:
  Coordinator(Bus &bus, const MarketData &data)
      : bus_{bus}, data_{data}, timer_{bus_.systemCtx()},
        statsRate_{ServerConfig::cfg.monitorRate} {
    bus_.marketBus.setHandler<ServerOrder>(
        [this](CRef<ServerOrder> order) { processOrder(order); });
  }

  void start() {
    startWorkers();
    scheduleStatsTimer();
  }

  void stop() {
    for (auto &worker : workers_) {
      worker->stop();
    }
  }

private:
  void startWorkers() {
    const auto appCores =
        ServerConfig::cfg.coresApp.empty() ? 1 : ServerConfig::cfg.coresApp.size();

    workers_.reserve(appCores);
    // Notify the system when all the workers have started
    const auto startCounter = std::make_shared<std::atomic_size_t>();
    const auto notifyClb = [this, startCounter, appCores]() {
      startCounter->fetch_add(1);
      if (startCounter->load() == appCores) {
        // Simplified notification for now that server is operational
        // ideally all the components start asynchronously and cc tracks the status
        bus_.systemBus.post(ServerEvent::Operational);
      };
    };
    if (ServerConfig::cfg.coresApp.empty()) {
      workers_.emplace_back(std::make_unique<CtxRunner>(0, false));
      workers_[0]->run();
      workers_[0]->ioCtx.post(notifyClb);
    } else {
      for (size_t i = 0; i < ServerConfig::cfg.coresApp.size(); ++i) {
        workers_.emplace_back(std::make_unique<CtxRunner>(i, true, ServerConfig::cfg.coresApp[i]));
        workers_[i]->run();
        workers_[i]->ioCtx.post(notifyClb);
      }
    }
  }

  void processOrder(CRef<ServerOrder> order) {
    LOG_TRACE(utils::toString(order));
    ordersTotal_.fetch_add(1, std::memory_order_relaxed);
    const auto &data = data_.at(order.order.ticker);
    workers_[data->getThreadId()]->ioCtx.post([this, order, &data]() {
      data->orderBook.add(order);
      data->orderBook.match(
          [this](CRef<ServerOrderStatus> status) { bus_.marketBus.post(status); });
    });
  }

  void scheduleStatsTimer() {
    timer_.expires_after(statsRate_);
    timer_.async_wait([this](BoostErrorCode ec) {
      if (ec) {
        LOG_ERROR_SYSTEM("Error {}", ec.message());
        return;
      }
      static uint64_t lastTtl = 0;
      const uint64_t currentTtl = ordersTotal_.load(std::memory_order_relaxed);
      const uint64_t rps = (currentTtl - lastTtl) / statsRate_.count();

      if (rps != 0) {
        const auto rpsStr = utils::thousandify(rps);
        const auto opnStr = utils::thousandify(countOpenedOrders());
        const auto ttlStr = utils::thousandify(currentTtl);

        LOG_INFO_SYSTEM("Orders: [opn|ttl] {}|{} | Rps: {}", opnStr, ttlStr, rpsStr);
      }
      lastTtl = currentTtl;
      scheduleStatsTimer();
    });
  }

  size_t countOpenedOrders() const {
    size_t orders = 0;
    for (auto &tickerData : data_) {
      orders += tickerData.second->orderBook.openedOrders();
    }
    return orders;
  }

private:
  Bus &bus_;
  const MarketData &data_;

  SteadyTimer timer_;
  Seconds statsRate_;

  std::atomic_uint64_t ordersTotal_;
  std::atomic_uint64_t ordersOpened_;

  std::vector<UPtr<CtxRunner>> workers_;
};

} // namespace hft::server

#endif // HFT_SERVER_COORDINATOR_HPP
