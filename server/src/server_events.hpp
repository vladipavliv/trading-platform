/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_SERVEREVENTS_HPP
#define HFT_SERVER_SERVEREVENTS_HPP

#include "bus/system_event_traits.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft::server {
/**
 * @brief Events to control the startup flow and other stuff
 * @details For now not really needed, but is needed in general
 */
enum class ServerEvent : uint8_t { Ready };

struct LoginRequestEvent {
  ObjectId connectionId;
  LoginRequest request;
};

struct LoginResultEvent {
  ObjectId connectionId;
  bool success{false};
  TraderId traderId{0};
};

} // namespace hft::server

namespace hft {
template <>
struct SystemEventTraits<server::LoginRequestEvent> {
  using KeyType = ObjectId;

  static KeyType getKey(CRef<server::LoginRequestEvent> event) { return event.connectionId; }
};
template <>
struct SystemEventTraits<server::LoginResultEvent> {
  using KeyType = ObjectId;

  static KeyType getKey(CRef<server::LoginResultEvent> event) { return event.connectionId; }
};
} // namespace hft

namespace hft::utils {
template <>
std::string toString<server::ServerEvent>(const server::ServerEvent &event) {
  using namespace server;
  switch (event) {
  case ServerEvent::Ready:
    return "ready";
  default:
    return std::format("unknown server event {}", static_cast<uint8_t>(event));
  }
}
} // namespace hft::utils

#endif // HFT_SERVER_SERVEREVENTS_HPP