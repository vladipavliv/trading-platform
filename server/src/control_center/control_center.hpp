/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_COMMON_CONTROL_CENTER_HPP
#define HFT_COMMON_CONTROL_CENTER_HPP

#include <deque>
#include <map>
#include <memory>
#include <spdlog/spdlog.h>

#include "boost_types.hpp"
#include "console/console_input_parser.hpp"
#include "logger_manager.hpp"
#include "market_types.hpp"
#include "server_types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::server {

/**
 * @brief Monitors console commands, prints stats, tunes the system
 * @details Could have used some timer for the needs, but
 */
class ControlCenter {
public:
  using Command = ServerControlSink::Command;
  using ConsoleParser = ConsoleInputParser<Command>;

  ControlCenter(ServerSink &sink)
      : mSink{sink}, mConsoleParser{{{"p+", Command::PriceFeedStart},
                                     {"p-", Command::PriceFeedStop},
                                     {"t", Command::GetTrafficStats},
                                     {"l+", Command::LogLevelUp},
                                     {"l-", Command::LogLevelDown},
                                     {"q", Command::Shutdown}}},
        mTimer{mSink.ctx()} {
    mSink.controlSink.addEventHandler<TrafficStats>([this](const TrafficStats &stats) {
      if (mStats.size() > 1) {
        mStats.pop_back();
      }
      mStats.push_front(stats);
    });
    scheduleTimer();
  }

private:
  void scheduleTimer() {
    mTimer.expires_after(MilliSeconds(100));
    mTimer.async_wait([this](BoostErrorRef ec) {
      if (!ec) {
        printMarketStats();
        processCommands();
        scheduleTimer();
      }
    });
  }

  void processCommands() {
    auto cmdRes = mConsoleParser.getCommand();
    while (cmdRes.ok()) {
      auto cmd = cmdRes.value();
      // Notify others. CC ain't subscribed so process those commands manually
      mSink.controlSink.onCommand(cmd);
      switch (cmd) {
      case Command::GetTrafficStats:
        mShowStats = !mShowStats;
        break;
      case Command::LogLevelUp:
        LoggerManager::switchLogLevel(true);
        break;
      case Command::LogLevelDown:
        LoggerManager::switchLogLevel(false);
        break;
      case Command::Shutdown:
      default:
        break;
      }
      cmdRes = mConsoleParser.getCommand();
    }
  }

private:
  void printMarketStats() {
    if (!mShowStats) {
      return;
    }
    if (mStats.empty()) {
      mSink.controlSink.onCommand(Command::GetTrafficStats);
    }
    auto now = std::chrono::system_clock::now();
    if (std::chrono::duration_cast<Seconds>(now - mStats.front().timestamp) < mStatsRate) {
      return;
    }
    // Request fresh stats by posting command. ControlSink is synchronous
    mSink.controlSink.onCommand(Command::GetTrafficStats);
    std::string log = std::format("Orders[opened,processed]:{}/{} ", mStats.front().currentOrders,
                                  mStats.front().processedOrders);
    if (mStats.size() > 1) {
      // Get time difference between last collected stats and now to calculate rps
      const uint32_t factor =
          std::chrono::duration_cast<Seconds>(mStats.front().timestamp - mStats.back().timestamp)
              .count();
      auto pRps = (mStats.front().processedOrders - mStats.back().processedOrders) / factor;
      log += std::format("RPS:{}", pRps);
    }
    spdlog::critical(log);
  }

  ServerSink &mSink;
  ConsoleParser mConsoleParser;

  std::deque<TrafficStats> mStats;
  bool mShowStats{false};
  Seconds mStatsRate{1};

  SteadyTimer mTimer;
};

} // namespace hft::server

#endif // HFT_COMMON_CONTROL_CENTER_HPP