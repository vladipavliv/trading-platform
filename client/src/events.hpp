/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_CLIENTEVENTS_HPP
#define HFT_SERVER_CLIENTEVENTS_HPP

#include <stdint.h>

#include "utils/string_utils.hpp"

namespace hft {
namespace client {
/**
 * @brief System events
 */
enum class ClientState : uint8_t { Connected, Disconnected, InternalError };
} // namespace client

namespace utils {
template <>
inline String toString<client::ClientState>(const client::ClientState &event) {
  using namespace client;
  switch (event) {
  case ClientState::Connected:
    return "Connected to the server";
  case ClientState::Disconnected:
    return "Disconnected from the server";
  case ClientState::InternalError:
    return "Internal error";
  default:
    return std::format("unknown event {}", static_cast<uint8_t>(event));
  }
}
} // namespace utils
} // namespace hft

#endif // HFT_SERVER_CLIENTEVENTS_HPP