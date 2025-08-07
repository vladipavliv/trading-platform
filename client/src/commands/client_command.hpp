/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_CLIENTCOMMAND_HPP
#define HFT_SERVER_CLIENTCOMMAND_HPP

#include <stdint.h>

#include "utils/string_utils.hpp"

namespace hft::client {
/**
 * @brief Command for console management
 */
enum class ClientCommand : uint8_t { Start, Stop, Telemetry_Start, Telemetry_Stop, Shutdown };
} // namespace hft::client

namespace hft::utils {

template <>
inline String toString<client::ClientCommand>(const client::ClientCommand &command) {
  using namespace client;
  switch (command) {
  case ClientCommand::Start:
    return "start";
  case ClientCommand::Stop:
    return "stop";
  case ClientCommand::Telemetry_Start:
    return "telemetry start";
  case ClientCommand::Telemetry_Stop:
    return "telemetry stop";
  case ClientCommand::Shutdown:
    return "shutdown";
  default:
    return std::format("unknown command {}", static_cast<uint8_t>(command));
  }
}

} // namespace hft::utils

#endif // HFT_SERVER_CLIENTCOMMAND_HPP
