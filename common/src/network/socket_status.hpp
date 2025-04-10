/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_SOCKETSTATUS_HPP
#define HFT_COMMON_SOCKETSTATUS_HPP

#include "market_types.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft {

enum class SocketStatus : uint8_t { Disconnected, Connected, Error };

struct SocketStatusEvent {
  const SocketId socketId;
  const SocketStatus status;
};

namespace utils {
String toString(const SocketStatus &status) {
  switch (status) {
  case SocketStatus::Disconnected:
    return "disconnected";
  case SocketStatus::Connected:
    return "connected";
  case SocketStatus::Error:
    return "error";
  default:
    return std::format("unknown status {}", static_cast<uint8_t>(status));
  }
}

String toString(const SocketStatusEvent &event) {
  return std::format("SocketStatusEvent Id:{} Status:{}", event.socketId, toString(event.status));
}

} // namespace utils
} // namespace hft

#endif // HFT_COMMON_SOCKETSTATUS_HPP
