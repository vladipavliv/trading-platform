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
  PriceFeedStart = 0U,
  PriceFeedStop = 1U,
  GetTrafficStats = 2U,
  LogLevelUp = 4U,
  LogLevelDown = 5U,
  Shutdown = 6U
};

} // namespace hft::server

namespace hft::utils {
template <>
std::string toString<server::ServerCommand>(const server::ServerCommand &cmd) {
  switch (cmd) {
  case server::ServerCommand::PriceFeedStart:
    return "start price feed";
  case server::ServerCommand::PriceFeedStop:
    return "stop price feed";
  case server::ServerCommand::GetTrafficStats:
    return "start/stop traffic stats";
  case server::ServerCommand::LogLevelUp:
    return "increase log level";
  case server::ServerCommand::LogLevelDown:
    return "decrese log level";
  case server::ServerCommand::Shutdown:
    return "shutdown";
  default:
    spdlog::error("Unknown ServerCommand {}", (uint8_t)cmd);
    return "";
  }
}
} // namespace hft::utils

#endif // HFT_SERVER_DISPATCHER_COMMAND_HPP