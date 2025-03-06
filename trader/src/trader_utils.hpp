/**
 * @author Vladimir Pavliv
 * @date 2025-02-20
 */

#ifndef HFT_SERVER_TRADERUTILS_HPP
#define HFT_SERVER_TRADERUTILS_HPP

#include "trader_command.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

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

#endif // HFT_SERVER_TRADERUTILS_HPP