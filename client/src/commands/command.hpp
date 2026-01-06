/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_CLIENT_COMMAND_HPP
#define HFT_CLIENT_COMMAND_HPP

#include <stdint.h>

#include "utils/string_utils.hpp"

namespace hft::client {
/**
 * @brief Command for console management
 */
enum class Command : uint8_t { Start, Stop, Telemetry_Start, Telemetry_Stop, Shutdown };
} // namespace hft::client

namespace hft::utils {

template <>
inline String toString<client::Command>(const client::Command &command) {
  using namespace client;
  switch (command) {
  case Command::Start:
    return "start";
  case Command::Stop:
    return "stop";
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

#endif // HFT_CLIENT_COMMAND_HPP
