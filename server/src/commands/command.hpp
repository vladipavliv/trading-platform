/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_COMMAND_HPP
#define HFT_SERVER_COMMAND_HPP

#include <stdint.h>

#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft::server {
/**
 * @brief Commands for console management
 * @todo Make commands to add/remove workers, orders rerouting
 */
enum class Command : uint8_t {
  PriceFeed_Start,
  PriceFeed_Stop,
  Telemetry_Start,
  Telemetry_Stop,
  Shutdown
};
} // namespace hft::server

namespace hft::utils {

template <>
inline String toString<server::Command>(const server::Command &command) {
  using namespace server;
  switch (command) {
  case Command::PriceFeed_Start:
    return "price feed start";
  case Command::PriceFeed_Stop:
    return "price feed stop";
  case Command::Telemetry_Start:
    return "telemetry start";
  case Command::Telemetry_Stop:
    return "telemetry stop";
  case Command::Shutdown:
    return "shutdown";
  default:
    return std::format("unknown command {}", static_cast<uint8_t>(command));
  }
}
} // namespace hft::utils

#endif // HFT_SERVER_SERVERCOMMAND_HPP
