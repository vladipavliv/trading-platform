/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_COMMAND_HPP
#define HFT_SERVER_COMMAND_HPP

#include <stdint.h>

#include "utils/utils.hpp"

namespace hft::server {

enum class ServerCommand : uint8_t {
  PrintMarketData = 0U,
  PriceFeedStart = 1U,
  PriceFeedStop = 2U,
  ShowMarketStats = 3U,
  ShowTrafficStats = 4U,
  Shutdown = 5U
};

} // namespace hft::server

namespace hft::utils {
template <>
std::string toString<server::ServerCommand>(const server::ServerCommand &cmd) {
  switch (cmd) {
  case server::ServerCommand::PrintMarketData:
    return "print market data";
  case server::ServerCommand::PriceFeedStart:
    return "start price feed";
  case server::ServerCommand::PriceFeedStop:
    return "stop price feed";
  case server::ServerCommand::ShowMarketStats:
    return "show stats";
  case server::ServerCommand::ShowTrafficStats:
    return "show stats";
  case server::ServerCommand::Shutdown:
    return "shutdown";
  default:
    spdlog::error("Unknown ServerCommand {}", (uint8_t)cmd);
    return "";
  }
}
} // namespace hft::utils

#endif // HFT_SERVER_DISPATCHER_COMMAND_HPP