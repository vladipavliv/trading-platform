/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_TRADERCOMMAND_HPP
#define HFT_SERVER_TRADERCOMMAND_HPP

#include <stdint.h>

#include "utils/string_utils.hpp"

namespace hft::trader {
/**
 * @brief Command for console management
 */
enum class TraderCommand : uint8_t {
  TradeStart,
  TradeStop,
  TradeSpeedUp,
  TradeSpeedDown,
  Shutdown
};
} // namespace hft::trader

namespace hft::utils {

template <>
std::string toString<trader::TraderCommand>(const trader::TraderCommand &command) {
  using namespace trader;
  switch (command) {
  case TraderCommand::TradeStart:
    return "trade start";
  case TraderCommand::TradeStop:
    return "trade stop";
  case TraderCommand::TradeSpeedUp:
    return "+ trade speed";
  case TraderCommand::TradeSpeedDown:
    return "- trade speed";
  case TraderCommand::Shutdown:
    return "shutdown";
  default:
    return std::format("unknown command {}", static_cast<uint8_t>(command));
  }
}

} // namespace hft::utils

#endif // HFT_SERVER_TRADERCOMMAND_HPP
