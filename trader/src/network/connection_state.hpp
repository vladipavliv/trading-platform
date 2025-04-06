/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_TRADER_CONNECTIONSTATE_HPP
#define HFT_TRADER_CONNECTIONSTATE_HPP

#include "utils/string_utils.hpp"

namespace hft {
namespace trader {
enum class ConnectionState : uint8_t {
  Disconnected,
  Connecting,
  Connected,
  AuthenticatedUpstream,
  AuthenticatedDownstream
};
}
namespace utils {
template <>
std::string toString<trader::ConnectionState>(const trader::ConnectionState &event) {
  using namespace trader;
  switch (event) {
  case ConnectionState::Disconnected:
    return "disconnected";
  case ConnectionState::Connecting:
    return "connecting";
  case ConnectionState::Connected:
    return "connected";
  case ConnectionState::AuthenticatedUpstream:
    return "authenticated upstream";
  case ConnectionState::AuthenticatedDownstream:
    return "authenticated downstream";
  default:
    return std::format("unknown event {}", static_cast<uint8_t>(event));
  }
}
} // namespace utils
} // namespace hft

#endif // HFT_TRADER_CONNECTIONSTATE_HPP