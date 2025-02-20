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
    auto avgRtt = stats.operations == 0 ? 0 : stats.rttSum / stats.operations;

    auto balance = std::format("Balance: ${}", static_cast<size_t>(stats.balance));
    auto avgStr = std::format("avg:{}", utils::getScaleNs(avgRtt));
    auto bestStr = std::format("best:{}", utils::getScaleNs(stats.rttBest));
    auto worstStr = std::format("worst:{}", utils::getScaleNs(stats.rttWorst));
    auto spikesStr = std::format("spikes:{}", stats.rttSpikes);

    auto log = std::format("{} Orders filled: {}   RTT {} {} {} {}", balance, stats.operations,
                           avgStr, bestStr, worstStr, spikesStr);
    LoggerManager::logService(log);
  }

private:
  TraderSink &mSink;
  ControlCenterBase<Command, TraderSink> mBase;
};

} // namespace hft::trader

#endif // HFT_TRADER_TRADERCONTROLCENTER_HPP
