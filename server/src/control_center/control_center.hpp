/**
 * @file monitor.hpp
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_COMMON_CONTROL_CENTER_HPP
#define HFT_COMMON_CONTROL_CENTER_HPP

#include <map>
#include <memory>
#include <spdlog/spdlog.h>

#include "boost_types.hpp"
#include "console/console_input_parser.hpp"
#include "market_types.hpp"
#include "server_types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::server {

class ControlCenter {
public:
  using Command = ControlSink::Command;
  using ConsoleParser = ConsoleInputParser<Command>;

  ControlCenter(ControlSink &sink)
      : mSink{sink}, mConsoleParser{{{"feed start", Command::MarketFeedStart},
                                     {"market start", Command::MarketFeedStart},
                                     {"feed stop", Command::MarketFeedStop},
                                     {"market start", Command::MarketFeedStop},
                                     {"stat", Command::ShowStats},
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

private:
  void consoleMonitor() {
    auto command = mConsoleParser.getCommand();
    while (command.ok()) {
      mSink.post(command.value());
      command = mConsoleParser.getCommand();
    }
  }

  void systemMonitor() { /* TODO(self): Make trace sink for monitoring all events? */ }

private:
  ControlSink &mSink;
  ConsoleParser mConsoleParser;
};

} // namespace hft::server

#endif // HFT_COMMON_CONTROL_CENTER_HPP