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
enum class ClientEvent : uint8_t { Connected, Disconnected, ConnectionFailed, InternalError };
} // namespace client

namespace utils {
template <>
inline String toString<client::ClientEvent>(const client::ClientEvent &event) {
  using namespace client;
  switch (event) {
  case ClientEvent::Connected:
    return "connected to the server";
  case ClientEvent::Disconnected:
    return "disconnected from the server";
  case ClientEvent::ConnectionFailed:
    return "failed to connect to the server";
  default:
    return std::format("unknown event {}", static_cast<uint8_t>(event));
  }
}
} // namespace utils
} // namespace hft

#endif // HFT_SERVER_CLIENTEVENTS_HPP