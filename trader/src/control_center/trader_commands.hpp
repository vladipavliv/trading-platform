/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_TRADER_COMMAND_HPP
#define HFT_TRADER_COMMAND_HPP

#include <stdint.h>

#include "utils/utils.hpp"

namespace hft::trader {

enum class TraderCommand : uint8_t {
  PlaceOrder = 0U,
  PriceFeedStart = 1U,
  PriceFeedStop = 2U,
  ShowStats = 3U,
  Shutdown = 4U
};

} // namespace hft::trader

namespace hft::utils {
template <>
std::string toString<trader::TraderCommand>(const trader::TraderCommand &cmd) {
  switch (cmd) {
  case trader::TraderCommand::PlaceOrder:
    return "place order";
  case trader::TraderCommand::PriceFeedStart:
    return "start price feed";
  case trader::TraderCommand::PriceFeedStop:
    return "stop price feed";
  case trader::TraderCommand::ShowStats:
    return "show stats";
  case trader::TraderCommand::Shutdown:
    return "shutdown";
  default:
    assert(0);
  }
}
} // namespace hft::utils

#endif // HFT_SERVER_DISPATCHER_COMMAND_HPP