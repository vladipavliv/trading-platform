/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_COMMAND_HPP
#define HFT_SERVER_COMMAND_HPP

#include <stdint.h>

#include "primitive_types.hpp"
#include "utils/string_utils.hpp"

namespace hft {
namespace server {
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
} // namespace server

inline String toString(const server::Command &command) {
  switch (command) {
  case server::Command::PriceFeed_Start:
    return "price feed start";
  case server::Command::PriceFeed_Stop:
    return "price feed stop";
  case server::Command::Telemetry_Start:
    return "telemetry start";
  case server::Command::Telemetry_Stop:
    return "telemetry stop";
  case server::Command::Shutdown:
    return "shutdown";
  default:
    return std::format("unknown command {}", static_cast<uint8_t>(command));
  }
}
} // namespace hft

#endif // HFT_SERVER_SERVERCOMMAND_HPP
