/**
 * @author Vladimir Pavliv
 * @date 2025-02-20
 */

#ifndef HFT_SERVER_COORDINATOR_HPP
#define HFT_SERVER_COORDINATOR_HPP

#include "bus/bus_hub.hpp"
#include "commands/command.hpp"
#include "config/server_config.hpp"
#include "container_types.hpp"
#include "domain/server_order_messages.hpp"
#include "domain_types.hpp"
#include "events.hpp"
#include "market_data.hpp"
#include "order_book.hpp"
#include "traits.hpp"
#include "utils/ctx_runner.hpp"
#include "utils/lfq_runner.hpp"
#include "utils/spin_wait.hpp"
#include "utils/string_utils.hpp"
#include "utils/time_utils.hpp"

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
      if (so.order.action == OrderAction::Dummy) {
        return;
      }
      const auto &oData = data.at(so.order.ticker);
      if (oData->orderBook.add(so, bus)) {
        oData->orderBook.match(bus);
      }
#if defined(BENCHMARK_BUILD) || defined(UNIT_TESTS_BUILD)
      oData->orderBook.sendAck(so, bus);
#endif
    }

    ServerBus &bus;
    const MarketData &data;
  };

  using Worker = LfqRunner<ServerOrder, Matcher>;

public:
  Coordinator(ServerBus &bus, CRef<MarketData> data) : bus_{bus}, matcher_{bus, data} {
    bus_.subscribe<ServerOrder>([this](CRef<ServerOrder> order) {
      if (matcher_.data.count(order.order.ticker) == 0) {
        LOG_ERROR_SYSTEM("Ticker not found {}", toString(order.order.ticker));
        return;
      }
      const auto &data = matcher_.data.at(order.order.ticker);
      auto &worker = workers_[data->getThreadId()];

      SpinWait waiter;
      while (!worker->post(order)) {
        if (!++waiter) {
          LOG_ERROR_SYSTEM("Failed to post order");
          break;
        }
      }
    });
    bus_.subscribe(Command::Telemetry_Start, [this] {
      LOG_INFO_SYSTEM("Start telemetry stream");
      telemetry_ = true;
    });
    bus_.subscribe(Command::Telemetry_Stop, [this] {
      LOG_INFO_SYSTEM("Stop telemetry stream");
      telemetry_ = false;
    });
  }

  void start() { startWorkers(); }

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

private:
  ServerBus &bus_;

  Matcher matcher_;
  bool telemetry_{false};

  Vector<UPtr<Worker>> workers_;
};

} // namespace hft::server

#endif // HFT_SERVER_COORDINATOR_HPP
