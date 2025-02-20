/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_TRADER_TRADERCONTROLCENTER_HPP
#define HFT_TRADER_TRADERCONTROLCENTER_HPP

#include <deque>
#include <map>
#include <memory>
#include <spdlog/spdlog.h>

#include "boost_types.hpp"
#include "console/control_center_base.hpp"
#include "logger_manager.hpp"
#include "rtt_tracker.hpp"
#include "strategy/trading_stats.hpp"
#include "trader_types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::trader {

class TraderControlCenter {
public:
  using Command = TraderControlSink::Command;

  TraderControlCenter(TraderSink &sink)
      : mSink{sink}, mBase{mSink,
                           {{"t+", Command::TradeStart},
                            {"t-", Command::TradeStop},
                            {"t", Command::TradeSwitch},
                            {"ts+", Command::TradeSpeedUp},
                            {"ts-", Command::TradeSpeedDown},
                            {"p+", Command::PriceFeedStart},
                            {"p-", Command::PriceFeedStop},
                            {"p", Command::PriceFeedSwitch},
                            {"m+", Command::MonitorModeStart},
                            {"m-", Command::MonitorModeStop},
                            {"m", Command::MonitorModeSwitch},
                            {"l+", Command::LogLevelUp},
                            {"l-", Command::LogLevelDown},
                            {"q", Command::Shutdown}}} {
    // Set stats collect callback it would be triggered synchronously
    // whenever we request them with command CollectStats
    mSink.controlSink.addEventHandler<TradingStats>(
        [this](const TradingStats &stats) { printStats(stats); });
  }

private:
  void printStats(const TradingStats &stats) {
    using namespace utils;
    auto rtt = RttTracker::getStats().samples;
    auto sampleSize = rtt[0].size + rtt[1].size + rtt[2].size;
    auto s0Rate = ((float)rtt[0].size / sampleSize) * 100;
    auto s1Rate = ((float)rtt[1].size / sampleSize) * 100;
    auto s2Rate = ((float)rtt[2].size / sampleSize) * 100;

    if (sampleSize == 0) {
      return;
    }
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "RTT [1us|100us|1ms]  " << s0Rate << "% avg:";
    ss << ((rtt[0].size != 0) ? (rtt[0].sum / rtt[0].size) : 0);
    ss << "us  " << s1Rate << "% avg:";
    ss << ((rtt[1].size != 0) ? (rtt[1].sum / rtt[1].size) : 0);
    ss << "us  " << s2Rate << "% avg:";
    ss << ((rtt[2].size != 0) ? ((rtt[2].sum / rtt[2].size) / 1000) : 0);
    ss << "ms";

    LoggerManager::logService(ss.str());
  }

private:
  TraderSink &mSink;
  ControlCenterBase<Command, TraderSink> mBase;
};

} // namespace hft::trader

#endif // HFT_TRADER_TRADERCONTROLCENTER_HPP
