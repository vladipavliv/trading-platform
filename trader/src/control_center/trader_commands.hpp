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
  TradeStart = 0U,     // trading start
  TradeStop = 1U,      // trading stop
  TradeSpeedUp = 3U,   // trade speed 2x
  TradeSpeedDown = 4U, // trade speed ½x
  CollectStats = 11U,  // start/stop switch for convenience
  LogLevelUp = 12U,    // logs level increase
  LogLevelDown = 13U,  // logs level decrease
  Shutdown = 14U       // shutdown
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

  case trader::TraderCommand::TradeSpeedUp:
    return "trade speed 2x";
  case trader::TraderCommand::TradeSpeedDown:
    return "trade speed ½x";

  case trader::TraderCommand::CollectStats:
    return "collect stats";

  case trader::TraderCommand::LogLevelUp:
    return "increase log level";
  case trader::TraderCommand::LogLevelDown:
    return "decrease log level";
  case trader::TraderCommand::Shutdown:
    return "shutdown";
  default:
    return "";
  }
}
} // namespace hft::utils

#endif // HFT_SERVER_DISPATCHER_COMMAND_HPP