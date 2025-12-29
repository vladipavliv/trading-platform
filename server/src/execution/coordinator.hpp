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
#include "lfq_runner.hpp"
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
  struct Matcher {
    Matcher(ServerBus &bus, const MarketData &data) : bus{bus}, data{data} {}

    inline void post(CRef<ServerOrder> so) {
      const auto &oData = data.at(so.order.ticker);
      if (oData.orderBook.add(so, bus)) {
        oData.orderBook.match(bus);
      }
#if defined(BENCHMARK_BUILD) || defined(UNIT_TESTS_BUILD)
      oData.orderBook.sendAck(so, bus);
#endif
#ifdef TELEMETRY_ENABLED
      ordersTotal.fetch_add(1, std::memory_order_relaxed);
#endif
    }

    ServerBus &bus;
    const MarketData &data;
    std::atomic_uint64_t ordersTotal;
  };

  using Worker = LfqRunner<ServerOrder, Matcher>;

public:
  Coordinator(ServerBus &bus, CRef<MarketData> data)
      : bus_{bus}, timer_{bus_.systemIoCtx()},
        monitorRate_{utils::toSeconds(ServerConfig::cfg.monitorRate)}, matcher_{bus, data} {
    bus_.subscribe<ServerOrder>([this](CRef<ServerOrder> order) {
      const auto &data = matcher_.data.at(order.order.ticker);
      workers_[data.getThreadId()]->post(order);
    });
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
    if (ServerConfig::cfg.coresApp.empty()) {
      workers_.emplace_back(std::make_unique<Worker>(matcher_, ErrorBus(bus_.systemBus)));
      workers_[0]->run();
    } else {
      for (size_t i = 0; i < ServerConfig::cfg.coresApp.size(); ++i) {
        const auto coreId = ServerConfig::cfg.coresApp[i];
        workers_.emplace_back(std::make_unique<Worker>(coreId, matcher_, ErrorBus(bus_.systemBus)));
        workers_[i]->run();
      }
    }
    bus_.systemBus.post(ServerEvent{ServerState::Operational, StatusCode::Ok});
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
      const uint64_t currentTtl = matcher_.ordersTotal.load(std::memory_order_relaxed);
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
    for (auto &it : matcher_.data) {
      orders += it.second.orderBook.openedOrders();
    }
    return orders;
  }

private:
  ServerBus &bus_;

  SteadyTimer timer_;
  const Seconds monitorRate_;

  Matcher matcher_;
  bool telemetry_{false};

  Vector<UPtr<Worker>> workers_;
};

} // namespace hft::server

#endif // HFT_SERVER_COORDINATOR_HPP
