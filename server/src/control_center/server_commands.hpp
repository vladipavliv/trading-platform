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
  PriceFeedStart = 0U,    // start generating prices
  PriceFeedStop = 1U,     // Stop generating prices
  PriceFeedSwitch = 2U,   // switch for convennience
  MonitorModeStart = 3U,  // start showing only statistics
  MonitorModeStop = 4U,   // switch back on to showing normal logs
  MonitorModeSwitch = 5U, // switch for convenience
  CollectStats = 6U,      // trigger traffic stats collecting
  LogLevelUp = 7U,        // increase log level
  LogLevelDown = 8U,      // decrease log level
  Shutdown = 9U           // shutdown
};

} // namespace hft::server

namespace hft::utils {
template <>
std::string toString<server::ServerCommand>(const server::ServerCommand &cmd) {
  switch (cmd) {
  case server::ServerCommand::PriceFeedStart:
    return "price feed start";
  case server::ServerCommand::PriceFeedStop:
    return "price feed stop";
  case server::ServerCommand::PriceFeedSwitch:
    return "price feed start/stop";

  case server::ServerCommand::MonitorModeStart:
    return "monitor mode on";
  case server::ServerCommand::MonitorModeStop:
    return "monitor mode off";
  case server::ServerCommand::MonitorModeSwitch:
    return "monitor mode on/off";

  case server::ServerCommand::CollectStats:
    return "collect stats";

  case server::ServerCommand::LogLevelUp:
    return "log level increase";
  case server::ServerCommand::LogLevelDown:
    return "log level decrese";

  case server::ServerCommand::Shutdown:
    return "shutdown";
  default:
    spdlog::error("Unknown ServerCommand {}", (uint8_t)cmd);
    return "";
  }
}
} // namespace hft::utils

#endif // HFT_SERVER_DISPATCHER_COMMAND_HPP