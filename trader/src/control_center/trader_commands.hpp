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
  StartTrading = 0U,
  StopTrading = 1U,
  PriceFeedStart = 2U,
  PriceFeedStop = 3U,
  ShowMarketStats = 4U,
  SwitchDebugMode = 5U,
  Shutdown = 6U
};

} // namespace hft::trader

namespace hft::utils {
template <>
std::string toString<trader::TraderCommand>(const trader::TraderCommand &cmd) {
  switch (cmd) {
  case trader::TraderCommand::StartTrading:
    return "start trading";
  case trader::TraderCommand::StopTrading:
    return "stop trading";
  case trader::TraderCommand::PriceFeedStart:
    return "start price feed";
  case trader::TraderCommand::PriceFeedStop:
    return "stop price feed";
  case trader::TraderCommand::ShowMarketStats:
    return "show stats";
  case trader::TraderCommand::SwitchDebugMode:
    return "switch debug on/off";
  case trader::TraderCommand::Shutdown:
    return "shutdown";
  default:
    spdlog::error("Unknown TraderCommand {}", (uint8_t)cmd);
    return "";
  }
}
} // namespace hft::utils

#endif // HFT_SERVER_DISPATCHER_COMMAND_HPP