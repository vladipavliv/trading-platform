/**
 * @file
 * @brief
 *
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
      : mSink{sink}, mConsoleParser{{{"order", Command::PlaceOrder},
                                     {"o", Command::PlaceOrder},
                                     {"feed start", Command::MarketFeedStart},
                                     {"market start", Command::MarketFeedStart},
                                     {"feed stop", Command::MarketFeedStop},
                                     {"market stop", Command::MarketFeedStop},
                                     {"stat", Command::ShowStats},
                                     {"statistics", Command::ShowStats},
                                     {"shutdown", Command::Shutdown},
                                     {"q", Command::Shutdown},
                                     {"exit", Command::Shutdown},
                                     {"stop", Command::Shutdown}}} {}

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
      auto cmdRes = mConsoleParser.getCommand();
      while (cmdRes.ok()) {
        auto cmd = cmdRes.value();
        spdlog::debug(utils::toString(cmd));
        mSink.controlSink.post(cmd);
        if (cmd == TraderCommand::Shutdown) {
          return;
        }
        if (cmd == TraderCommand::PlaceOrder) {
          mSink.networkSink.post(utils::generateOrder());
        }
        cmdRes = mConsoleParser.getCommand();
      }
      boost::this_fiber::yield();
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
