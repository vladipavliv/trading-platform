/**
 * @author Vladimir Pavliv
 * @date 2025-02-20
 */

#ifndef HFT_SERVER_COORDINATOR_HPP
#define HFT_SERVER_COORDINATOR_HPP

#include "commands/server_command.hpp"
#include "config/server_config.hpp"
#include "ctx_runner.hpp"
#include "domain_types.hpp"
#include "order_book.hpp"
#include "server_events.hpp"
#include "server_market_data.hpp"
#include "server_types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::server {

/**
 * @brief Redirects Orders to a proper worker
 * @todo Later on would perform ticker rerouting and dynamic worker add/remove
 * Suggested rerouting approach:
 * MarketData container is const as tickers are read at a startup and never change
 * so workers can safely work with it concurrently.
 * When rerouting happens, atomic worker id gets changed in TickerData, so new orders
 * go to the new worker queue. For the orders that are already in the previous workers
 * queue, they would have to be redirected via ::dispatch to a new workers context.
 * Additionally, OrderBook would require atomic flag to lock it and not allow redirections
 * when worker is updating it. So worker locks the book, checks its worker id,
 * if its not the same as its id, it redirects the order to a new worker.
 * If old worker tries to lock the book, when its already locked, there could be two cases:
 * - system thread is in the middle of rerouting
 * - new worker is already processing new orders
 * In the first case rerouting happens for a very brief moment, its just 3 atomic operations
 * to lock the book, change worker id, and unlock it. For the second case, it could take longer
 * So old worker sees that the book is locked, and checks the new worker id.
 * If its the same as its id, then it caught system thread in the middle of the update,
 * and it can spin for a few cycles waiting when worker id changes.
 * This, supposedly, would not have much effect on the workers performance
 * and it wont have to spin for too long.
 * Once worker id changes - old worker reposts the order to a new one.
 * LLM idea improvement: instead of using separate flag to lock the book, embed this into
 * the workerId atomic. Have uint64 instead of uint32, and use 4 bytes to store worker id,
 * and another 4 bytes to indicate that the book is locked.
 * So when worker tries to process the order, it does compare_exchange, and checks for the
 * value to have its id, and flag 'free', and it exchanges it with its id and flag 'busy'
 */
class Coordinator {
public:
  Coordinator(ServerBus &bus, CRef<MarketData> data)
      : bus_{bus}, data_{data}, timer_{bus_.systemIoCtx()},
        monitorRate_{utils::toSeconds(ServerConfig::cfg.monitorRate)} {
    bus_.subscribe<ServerOrder>([this](CRef<ServerOrder> order) { processOrder(order); });
    bus_.subscribe(ServerCommand::Telemetry_Start, [this] {
      LOG_INFO_SYSTEM("Start telemetry stream");
      telemetry_ = true;
    });
    bus_.subscribe(ServerCommand::Telemetry_Stop, [this] {
      LOG_INFO_SYSTEM("Stop telemetry stream");
      telemetry_ = false;
    });
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
        bus_.systemBus.post(ServerEvent{ServerState::Operational, StatusCode::Ok});
      };
    };
    if (ServerConfig::cfg.coresApp.empty()) {
      workers_.emplace_back(std::make_unique<CtxRunner>(ErrorBus(bus_.systemBus)));
      workers_[0]->run();
      workers_[0]->ioCtx.post(notifyClb);
    } else {
      for (size_t i = 0; i < ServerConfig::cfg.coresApp.size(); ++i) {
        workers_.emplace_back(std::make_unique<CtxRunner>(i, ServerConfig::cfg.coresApp[i],
                                                          ErrorBus(bus_.systemBus)));
        workers_[i]->run();
        workers_[i]->ioCtx.post(notifyClb);
      }
    }
  }

  void processOrder(CRef<ServerOrder> so) {
    LOG_TRACE("{}", utils::toString(so));
    const auto &data = data_.at(so.order.ticker);
    workers_[data.getThreadId()]->ioCtx.post([this, so, &data]() {
      if (data.orderBook.add(so, bus_)) {
        data.orderBook.match(bus_);
      }
      ordersTotal_.fetch_add(1, std::memory_order_relaxed);
    });
  }

  void scheduleStatsTimer() {
    timer_.expires_after(monitorRate_);
    timer_.async_wait([this](BoostErrorCode ec) {
      if (ec) {
        if (ec != ASIO_ERR_ABORTED) {
          LOG_ERROR_SYSTEM("Error {}", ec.message());
        }
        return;
      }
      static std::atomic_uint64_t lastTtl = 0;
      const uint64_t currentTtl = ordersTotal_.load(std::memory_order_relaxed);
      const uint64_t rps = (currentTtl - lastTtl) / monitorRate_.count();

      if (rps != 0) {
        LOG_INFO_SYSTEM("Orders: [opn|ttl] {}|{} | Rps: {}", openedOrders(), currentTtl, rps);
#ifdef TELEMETRY_ENABLED
        if (telemetry_) {
          bus_.post(RuntimeMetrics{MetadataSource::Server, utils::getTimestamp(), rps, 0});
        }
#endif
      }
      lastTtl = currentTtl;
      scheduleStatsTimer();
    });
  }

  size_t openedOrders() const {
    size_t orders{0};
    for (auto &it : data_) {
      orders += it.second.orderBook.openedOrders();
    }
    return orders;
  }

private:
  ServerBus &bus_;
  const MarketData &data_;

  SteadyTimer timer_;
  const Seconds monitorRate_;

  std::atomic_uint64_t ordersTotal_;
  std::atomic_uint64_t ordersOpened_;

  bool telemetry_{false};

  Vector<UPtr<CtxRunner>> workers_;
};

} // namespace hft::server

#endif // HFT_SERVER_COORDINATOR_HPP
