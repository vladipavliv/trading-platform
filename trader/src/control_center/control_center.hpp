/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_TRADER_CONTROL_CENTER_HPP
#define HFT_TRADER_CONTROL_CENTER_HPP

#include <map>
#include <memory>
#include <spdlog/spdlog.h>

#include "boost_types.hpp"
#include "console/console_input_parser.hpp"
#include "logger_manager.hpp"
#include "market_types.hpp"
#include "trader_types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::trader {

class ControlCenter {
public:
  using Command = TraderControlSink::Command;
  using ConsoleParser = ConsoleInputParser<Command>;

  ControlCenter(TraderSink &sink)
      : mSink{sink}, mConsoleParser{{{"t+", Command::TradeStart},
                                     {"t-", Command::TradeStop},
                                     {"p+", Command::PriceFeedStart},
                                     {"p-", Command::PriceFeedStop},
                                     {"l+", Command::LogLevelUp},
                                     {"l-", Command::LogLevelDown},
                                     {"q", Command::Shutdown}}},
        mTimer{mSink.ctx()} {
    scheduleTimer();
  }

private:
  void scheduleTimer() {
    mTimer.expires_after(MilliSeconds(100));
    mTimer.async_wait([this](BoostErrorRef ec) {
      if (!ec) {
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
  TraderSink &mSink;
  ConsoleParser mConsoleParser;

  SteadyTimer mTimer;
};

} // namespace hft::trader

#endif // HFT_COMMON_CONTROL_CENTER_HPP
