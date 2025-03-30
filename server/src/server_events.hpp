/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_SERVEREVENTS_HPP
#define HFT_SERVER_SERVEREVENTS_HPP

#include "bus/system_event_traits.hpp"
#include "network/session_status.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft {
namespace server {
/**
 * @brief Events to control the startup flow and other stuff
 * @details For now not really needed, but is needed in general
 */
enum class ServerEvent : uint8_t { Ready };

struct SessionStatusEvent {
  ObjectId connectionId{0};
  TraderId traderId{0};
  SessionStatus status{SessionStatus::Disconnected};
};

struct LoginRequestEvent {
  ObjectId connectionId{0};
  LoginRequest request;
};

struct LoginResponseEvent {
  ObjectId connectionId{0};
  TraderId traderId{0};
  LoginResponse response;
};
} // namespace server

template <>
struct SystemEventTraits<server::SessionStatusEvent> {
  using KeyType = ObjectId;
  static KeyType getKey(CRef<server::SessionStatusEvent> event) { return event.traderId; }
};

template <>
struct SystemEventTraits<server::LoginRequestEvent> {
  using KeyType = ObjectId;
  static KeyType getKey(CRef<server::LoginRequestEvent> event) { return event.connectionId; }
};

template <>
struct SystemEventTraits<server::LoginResponseEvent> {
  using KeyType = ObjectId;
  static KeyType getKey(CRef<server::LoginResponseEvent> event) { return event.connectionId; }
};

namespace utils {
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

template <>
std::string toString<server::LoginRequestEvent>(const server::LoginRequestEvent &e) {
  return std::format("LoginRequestEvent {} {}", e.connectionId, utils::toString(e.request));
}

template <>
std::string toString<server::LoginResponseEvent>(const server::LoginResponseEvent &e) {
  return std::format("LoginResponseEvent {} {} {}", e.connectionId, e.traderId,
                     utils::toString(e.response));
}

} // namespace utils
} // namespace hft

#endif // HFT_SERVER_SERVEREVENTS_HPP