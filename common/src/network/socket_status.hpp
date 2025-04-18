/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_CONNECTIONSTATUS_HPP
#define HFT_COMMON_CONNECTIONSTATUS_HPP

#include "domain_types.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft {

enum class ConnectionStatus : uint8_t { Disconnected, Connected, Error };

struct ConnectionStatusEvent {
  const ConnectionId connectionId;
  const ConnectionStatus status;
};

namespace utils {
String toString(const ConnectionStatus &status) {
  switch (status) {
  case ConnectionStatus::Disconnected:
    return "disconnected";
  case ConnectionStatus::Connected:
    return "connected";
  case ConnectionStatus::Error:
    return "error";
  default:
    return std::format("unknown status {}", static_cast<uint8_t>(status));
  }
}

String toString(const ConnectionStatusEvent &event) {
  return std::format("ConnectionStatusEvent connectionId:{} Status:{}", event.connectionId,
                     toString(event.status));
}

} // namespace utils
} // namespace hft

#endif // HFT_COMMON_CONNECTIONSTATUS_HPP
