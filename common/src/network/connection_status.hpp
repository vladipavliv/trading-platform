/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_CONNECTIONSTATUS_HPP
#define HFT_COMMON_CONNECTIONSTATUS_HPP

#include "bus/system_event_traits.hpp"
#include "market_types.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft {

enum class ConnectionStatus : uint8_t { Disconnected, Connected, Error };

struct TcpStatusEvent {
  ObjectId id;
  TraderId traderId;
  ConnectionStatus status;
};

struct UdpStatusEvent {
  ObjectId id;
  ConnectionStatus status;
};

template <>
struct SystemEventTraits<TcpStatusEvent> {
  using KeyType = ObjectId;
  static KeyType getKey(CRef<TcpStatusEvent> event) { return event.id; }
};

template <>
struct SystemEventTraits<UdpStatusEvent> {
  using KeyType = ObjectId;
  static KeyType getKey(CRef<UdpStatusEvent> event) { return event.id; }
};

} // namespace hft

namespace hft::utils {

template <>
std::string toString<ConnectionStatus>(const ConnectionStatus &status) {
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

} // namespace hft::utils

#endif // HFT_COMMON_CONNECTIONSTATUS_HPP