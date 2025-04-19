/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_CLIENT_CONNECTIONSTATE_HPP
#define HFT_CLIENT_CONNECTIONSTATE_HPP

#include "utils/string_utils.hpp"

namespace hft {
namespace client {
/**
 * @brief State of the connection with the server
 */
enum class ConnectionState : uint8_t {
  Disconnected,
  Connecting,
  Connected,
  TokenReceived,
  Authenticated
};
} // namespace client
namespace utils {
String toString(const client::ConnectionState &event) {
  using namespace client;
  switch (event) {
  case ConnectionState::Disconnected:
    return "disconnected";
  case ConnectionState::Connecting:
    return "connecting";
  case ConnectionState::Connected:
    return "connected";
  case ConnectionState::TokenReceived:
    return "token received";
  case ConnectionState::Authenticated:
    return "authenticated";
  default:
    return std::format("unknown event {}", static_cast<uint8_t>(event));
  }
}
} // namespace utils
} // namespace hft

#endif // HFT_CLIENT_CONNECTIONSTATE_HPP