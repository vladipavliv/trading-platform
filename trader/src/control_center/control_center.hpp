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
#include "market_types.hpp"
#include "trader_types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::trader {

class ControlCenter {
public:
  using Command = ControlSink::Command;
  using ConsoleParser = ConsoleInputParser<Command>;

  ControlCenter(TraderSink &sink)
      : mSink{sink}, mConsoleParser{{{"t+", Command::StartTrading},
                                     {"t-", Command::StopTrading},
                                     {"f+", Command::PriceFeedStart},
                                     {"f-", Command::PriceFeedStop},
                                     {"d", Command::SwitchDebugMode},
                                     {"q", Command::Shutdown}}} {}

  void start() {
    Fiber consoleFiber([this]() { consoleMonitor(); });
    Fiber monitorFiber([this]() { systemMonitor(); });

    consoleFiber.join();
    monitorFiber.join();
  }

  void stop() { mStop.store(true); }

private:
  void consoleMonitor() {
    while (!mStop.load()) {
      boost::this_fiber::yield();
      auto cmdRes = mConsoleParser.getCommand();
      if (!cmdRes.ok()) {
        continue;
      }
      auto cmd = cmdRes.value();
      mSink.controlSink.post(cmd);
      if (cmd == TraderCommand::Shutdown) {
        return;
      }
      if (cmd == TraderCommand::SwitchDebugMode) {
        if (spdlog::get_level() == spdlog::level::debug) {
          spdlog::set_level(spdlog::level::info);
        } else {
          spdlog::set_level(spdlog::level::debug);
        }
      }
      cmdRes = mConsoleParser.getCommand();
    }
  }

  void systemMonitor() { /* TODO(self): Make trace sink for monitoring all events? */ }

private:
  TraderSink &mSink;
  ConsoleParser mConsoleParser;

  std::atomic_bool mStop{false};
};

} // namespace hft::trader

#endif // HFT_COMMON_CONTROL_CENTER_HPP
