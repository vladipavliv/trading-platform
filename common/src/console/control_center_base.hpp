/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_COMMON_CONTROLCENTERBASE_HPP
#define HFT_COMMON_CONTROLCENTERBASE_HPP

#include <deque>
#include <map>
#include <memory>
#include <spdlog/spdlog.h>

#include "boost_types.hpp"
#include "config/config.hpp"
#include "console_input_parser.hpp"
#include "logger.hpp"
#include "market_types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Base for control center class to periodically check for commands from console
 */
template <typename CommandType, typename SinkType>
class ControlCenterBase {
public:
  using Command = CommandType;
  using Sink = SinkType;
  using ConsoleParser = ConsoleInputParser<Command>;

  ControlCenterBase(Sink &sink, std::map<std::string, Command> &&cmdMap)
      : mSink{sink}, mConsoleParser{std::move(cmdMap)}, mTimer{mSink.ctx()} {
    scheduleTimer();
  }

private:
  void scheduleTimer() {
    mTimer.expires_after(MilliSeconds(100));
    mTimer.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        return;
      }
      processCommands();
      scheduleTimer();
    });
  }

  void processCommands() {
    auto cmdRes = mConsoleParser.getCommand();
    while (cmdRes.ok()) {
      auto cmd = cmdRes.value;
      switch (cmd) {
      case Command::LogLevelUp:
        Logger::switchLogLevel(true);
        Logger::monitorLogger->info("log level {}", utils::toString(spdlog::get_level()));
        break;
      case Command::LogLevelDown:
        Logger::switchLogLevel(false);
        Logger::monitorLogger->info("log level {}", utils::toString(spdlog::get_level()));
        break;
      default:
        Logger::monitorLogger->info(utils::toString(cmd));
        mSink.controlSink.onCommand(cmd);
        break;
      }
      cmdRes = mConsoleParser.getCommand();
    }
  }

private:
  Sink &mSink;
  ConsoleParser mConsoleParser;

  SteadyTimer mTimer;
};

} // namespace hft

#endif // HFT_COMMON_CONTROLCENTERBASE_HPP