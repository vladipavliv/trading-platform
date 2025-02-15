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
  PriceFeedStart = 0U,
  PriceFeedStop = 1U,
  ShowStats = 2U,
  Shutdown = 3U
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
  case server::ServerCommand::ShowStats:
    return "show stats";
  case server::ServerCommand::Shutdown:
    return "shutdown";
  default:
    assert(0);
  }
}
} // namespace hft::utils

#endif // HFT_SERVER_DISPATCHER_COMMAND_HPP