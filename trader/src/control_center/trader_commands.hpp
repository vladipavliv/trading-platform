/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_TRADER_COMMAND_HPP
#define HFT_TRADER_COMMAND_HPP

#include <spdlog/spdlog.h>
#include <stdint.h>

#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::trader {

enum class TraderCommand : uint8_t {
  TradeStart = 0U,         // trading start
  TradeStop = 1U,          // trading stop
  TradeSwitch = 2U,        // switch for convenience
  TradeSpeedUp = 3U,       // trade speed 2x
  TradeSpeedDown = 4U,     // trade speed ½x
  PriceFeedStart = 5U,     // price feed show
  PriceFeedStop = 6U,      // price feed hide
  PriceFeedSwitch = 7U,    // switch for convenience
  MonitorModeStart = 8U,   // start showing only statistics
  MonitorModeStop = 9U,    // switch back on to showing normal logs
  MonitorModeSwitch = 10U, // start/stop switch for convenience
  CollectStats = 11U,      // start/stop switch for convenience
  LogLevelUp = 12U,        // logs level increase
  LogLevelDown = 13U,      // logs level decrease
  Shutdown = 14U           // shutdown
};

} // namespace hft::trader

namespace hft::utils {
template <>
std::string toString<trader::TraderCommand>(const trader::TraderCommand &cmd) {
  switch (cmd) {
  case trader::TraderCommand::TradeStart:
    return "trade start";
  case trader::TraderCommand::TradeStop:
    return "trade stop";
  case trader::TraderCommand::TradeSwitch:
    return "trade start/stop";

  case trader::TraderCommand::TradeSpeedUp:
    return "trade speed 2x";
  case trader::TraderCommand::TradeSpeedDown:
    return "trade speed ½x";

  case trader::TraderCommand::PriceFeedStart:
    return "price feed show";
  case trader::TraderCommand::PriceFeedStop:
    return "price feed stop";
  case trader::TraderCommand::PriceFeedSwitch:
    return "price feed show/hide";

  case trader::TraderCommand::MonitorModeStart:
    return "monitor mode on";
  case trader::TraderCommand::MonitorModeStop:
    return "monitor mode off";
  case trader::TraderCommand::MonitorModeSwitch:
    return "monitor mode on/off";

  case trader::TraderCommand::CollectStats:
    return "collect stats";

  case trader::TraderCommand::LogLevelUp:
    return "increase log level";
  case trader::TraderCommand::LogLevelDown:
    return "decrease log level";
  case trader::TraderCommand::Shutdown:
    return "shutdown";
  default:
    spdlog::error("Unknown TraderCommand {}", (uint8_t)cmd);
    return "";
  }
}
} // namespace hft::utils

#endif // HFT_SERVER_DISPATCHER_COMMAND_HPP