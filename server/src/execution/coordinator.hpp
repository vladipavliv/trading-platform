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
#include "gateway/internal_order.hpp"
#include "market_data.hpp"
#include "traits.hpp"
#include "utils/ctx_runner.hpp"
#include "utils/lfq_runner.hpp"
#include "utils/spin_wait.hpp"
#include "utils/string_utils.hpp"
#include "utils/time_utils.hpp"

namespace hft::server {

/**
 * @brief Manages order-matching workers, routes order to a proper worker by the ticker
 * for optimization tickerdata is supplied with InternalOrderEvent so the worker doesnt have to look
 * it up again in the MarketData
 * for further optimizations all tickers could be indexed at a startup to avoid 'ticker->' hashmap
 * @note ticker rerouting could be done by managing WorkerId uint32 atomic,
 * using highest bit to indicate that book is locked
 * 1. on ticker reroute sys thread does spin CAS in the OrderBook
 *    (free_flag|curr_worker_id)->(free_flag|next_worker_id)
 * 2. on order processing worker does CAS
 *    (free_flag|self_id)->(busy_flag|self_id)
 *    only way it fails is if worker_id changed
 * 3. on failure order gets returned to Coordinator for dispatching to a new worker
 * 4. on success order gets processed blocking attempts to reroute the ticker
 */
class Coordinator {
  /**
   * @brief Consumer for workers to execute order in their thread
   */
  struct Matcher {
    explicit Matcher(ServerBus &bus) : bus{bus} {}

    inline void post(CRef<InternalOrderEvent> ioe) {
      LOG_DEBUG("Matcher {}", toString(ioe));
      ioe.data->orderBook.add(ioe, bus);
    }

    ServerBus &bus;
  };
  using Worker = LfqRunner<InternalOrderEvent, Matcher, SystemBus>;

public:
  Coordinator(Context &ctx, CRef<MarketData> data) : ctx_{ctx}, data_{data}, matcher_{ctx_.bus} {
    ctx_.bus.subscribe<InternalOrderEvent>(
        CRefHandler<InternalOrderEvent>::template bind<Coordinator, &Coordinator::post>(this));
  }

  ~Coordinator() { LOG_DEBUG_SYSTEM("~Coordinator"); }

  void start() {
    LOG_DEBUG("Coordinator start");
    startWorkers();
  }

  void stop() {
    LOG_DEBUG("Coordinator stop");
    for (auto &worker : workers_) {
      worker->stop();
    }
  }

private:
  void startWorkers() {
    LOG_DEBUG("Coordinator::startWorkers");
    if (started_.load()) {
      LOG_ERROR_SYSTEM("Already started");
      return;
    }
    const auto appCores = ctx_.config.coresApp.empty() ? 1 : ctx_.config.coresApp.size();

    started_.store(true);
    workers_.reserve(appCores);
    if (ctx_.config.coresApp.empty()) {
      workers_.emplace_back(
          std::make_unique<Worker>(matcher_, ctx_.bus.systemBus, ctx_.stopToken, "worker zero"));
      workers_[0]->run();
    } else {
      for (size_t i = 0; i < ctx_.config.coresApp.size(); ++i) {
        const auto name = std::format("worker {}", i);
        const auto coreId = ctx_.config.coresApp[i];
        workers_.emplace_back(
            std::make_unique<Worker>(matcher_, ctx_.bus.systemBus, ctx_.stopToken, name, coreId));
        workers_[i]->run();
      }
    }
    ctx_.bus.post(ServerEvent{ServerState::Operational, StatusCode::Ok});
  }

  void post(CRef<InternalOrderEvent> ioe) {
    if (ctx_.stopToken.stop_requested()) {
      LOG_WARN_SYSTEM("Coordinator already stopped");
      return;
    }
    if (data_.count(ioe.ticker) == 0) {
      LOG_ERROR_SYSTEM("Ticker not found {}", toString(ioe.ticker));
      return;
    }
    ioe.data = &data_.at(ioe.ticker);
    workers_[ioe.data->workerId]->post(ioe);
  }

private:
  Context &ctx_;

  const MarketData &data_;

  AtomicBool started_{false};
  Matcher matcher_;
  Vector<UPtr<Worker>> workers_;
};

} // namespace hft::server

#endif // HFT_SERVER_COORDINATOR_HPP
