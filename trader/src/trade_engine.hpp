/**
 * @author Vladimir Pavliv
 * @date 2025-04-06
 */

#ifndef HFT_SERVER_TRADEENGINE_HPP
#define HFT_SERVER_TRADEENGINE_HPP

#include "bus/bus.hpp"
#include "config/config.hpp"

namespace hft::trader {

class TradeEngine {
public:
  explicit TradeEngine(Bus &bus) : bus_{bus} {}

  void start() {
    startWorkers();
    scheduleStatsTimer();
  }

  void stop() {
    for (auto &worker : workers_) {
      worker->ioCtx.stop();
    }
  }

private:
  void startWorkers() {
    const auto appCores = Config::cfg.coresApp.size();
    LOG_INFO_SYSTEM("Starting {} workers", appCores);
    workers_.reserve(appCores);
    for (int i = 0; i < appCores; ++i) {
      workers_.emplace_back(std::make_unique<Worker>(i, Config::cfg.coresApp[i]));
    }
  }

private:
  Bus &bus_;

  std::vector<Worker::UPtr> workers_;
};
} // namespace hft::trader

#endif // HFT_SERVER_TRADEENGINE_HPP