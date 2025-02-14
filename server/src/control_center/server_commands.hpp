/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_COMMAND_HPP
#define HFT_SERVER_COMMAND_HPP

#include <stdint.h>

#include "utils/utils.hpp"

namespace hft::server {

enum class ServerCommand : uint8_t {
  MarketFeedStart = 0U,
  MarketFeedStop = 1U,
  ShowStats = 2U,
  Shutdown = 3U
};

} // namespace hft::server

namespace hft::utils {
template <>
std::string toString<server::ServerCommand>(const server::ServerCommand &cmd) {
  switch (cmd) {
  case server::ServerCommand::MarketFeedStart:
    return "start market feed";
  case server::ServerCommand::MarketFeedStop:
    return "stop market feed";
  case server::ServerCommand::ShowStats:
    return "show stats";
  case server::ServerCommand::Shutdown:
    return "shutdown";
  default:
    spdlog::error("Unknown order action");
    throw std::runtime_error("Unknown order action");
  }
}
} // namespace hft::utils

#endif // HFT_SERVER_DISPATCHER_COMMAND_HPP