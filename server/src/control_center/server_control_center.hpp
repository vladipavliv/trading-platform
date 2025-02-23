/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_COMMON_CONTROL_CENTER_HPP
#define HFT_COMMON_CONTROL_CENTER_HPP

#include <deque>
#include <format>
#include <map>
#include <memory>
#include <spdlog/spdlog.h>

#include "aggregator/order_traffic_stats.hpp"
#include "boost_types.hpp"
#include "console/control_center_base.hpp"
#include "logger.hpp"
#include "market_types.hpp"
#include "server_types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::server {

/**
 * @brief Monitors console commands, prints stats, tunes the system
 */
class ServerControlCenter {
public:
  using Command = ServerControlSink::Command;

  ServerControlCenter(ServerSink &sink)
      : mSink{sink}, mBase{mSink,
                           {{"p+", Command::PriceFeedStart},
                            {"p-", Command::PriceFeedStop},
                            {"l+", Command::LogLevelUp},
                            {"l-", Command::LogLevelDown},
                            {"q", Command::Shutdown}}},
        mMonitorRate{Config::cfg.monitorRateS}, mTimer{mSink.ctx()} {
    // Set stats collect callback,
    // it would be triggered synchronously whenever we request them with command CollectStats
    mSink.controlSink.addEventHandler<OrderTrafficStats>([this](const OrderTrafficStats &stats) {
      // Just keep the last two for RPS stats
      if (mStats.size() > 1) {
        mStats.pop_back();
      }
      mStats.push_front(stats);
      printStats();
    });
    scheduleTimer();
  }

private:
  void scheduleTimer() {
    mTimer.expires_after(Seconds(mMonitorRate));
    mTimer.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        return;
      }
      mSink.controlSink.onCommand(Command::CollectStats);
      scheduleTimer();
    });
  }

  void printStats() {
    if (mStats.size() < 2 || mStats.front() == mStats.back()) {
      return;
    }

    std::string log = std::format("[open|total]: {:L}|{:L} ", mStats.front().currentOrders,
                                  mStats.front().processedOrders);
    if (mStats.size() > 1) {
      // Get time difference between last collected stats and now to calculate rps
      const uint32_t factor =
          std::chrono::duration_cast<Seconds>(mStats.front().timestamp - mStats.back().timestamp)
              .count();
      auto pRps = (mStats.front().processedOrders - mStats.back().processedOrders) / factor;
      log += std::format("RPS:{}", pRps);
    }
    Logger::monitorLogger->info(log);
  }

private:
  ServerSink &mSink;
  ControlCenterBase<Command, ServerSink> mBase;

  std::deque<OrderTrafficStats> mStats;
  Seconds mMonitorRate;
  SteadyTimer mTimer;
};

} // namespace hft::server

#endif // HFT_COMMON_CONTROL_CENTER_HPP