/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_SESSIONSTATUS_HPP
#define HFT_COMMON_SESSIONSTATUS_HPP

#include "bus/system_event_traits.hpp"
#include "market_types.hpp"
#include "socket_status.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft {
enum class SessionStatus : uint8_t { Disconnected, Connected, Authenticated, Error };

namespace utils {
template <>
std::string toString<SessionStatus>(const SessionStatus &status) {
  switch (status) {
  case SessionStatus::Disconnected:
    return "disconnected";
  case SessionStatus::Connected:
    return "connected";
  case SessionStatus::Authenticated:
    return "authenticated";
  case SessionStatus::Error:
    return "error";
  default:
    return std::format("unknown status {}", static_cast<uint8_t>(status));
  }
}
} // namespace utils
} // namespace hft

#endif // HFT_COMMON_SESSIONSTATUS_HPP