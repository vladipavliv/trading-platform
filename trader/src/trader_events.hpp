/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_TRADEREVENTS_HPP
#define HFT_SERVER_TRADEREVENTS_HPP

#include <stdint.h>

#include "network/session_status.hpp"
#include "utils/string_utils.hpp"

namespace hft {
namespace trader {
/**
 * @brief System events
 */
enum class TraderEvent : uint8_t { Initialized, Connected, Disconnected };

struct SessionStatusEvent {
  ObjectId connectionId{0};
  SessionStatus status{SessionStatus::Disconnected};
};

struct LoginRequestEvent {
  ObjectId connectionId{0};
  LoginRequest request;
};

struct LoginResponseEvent {
  ObjectId connectionId{0};
  LoginResponse response;
};
} // namespace trader

template <>
struct SystemEventTraits<trader::SessionStatusEvent> {
  using KeyType = ObjectId;
  static KeyType getKey(CRef<trader::SessionStatusEvent> event) { return event.connectionId; }
};

template <>
struct SystemEventTraits<trader::LoginRequestEvent> {
  using KeyType = ObjectId;
  static KeyType getKey(CRef<trader::LoginRequestEvent> event) { return event.connectionId; }
};

template <>
struct SystemEventTraits<trader::LoginResponseEvent> {
  using KeyType = ObjectId;
  static KeyType getKey(CRef<trader::LoginResponseEvent> event) { return event.connectionId; }
};

namespace utils {
template <>
std::string toString<trader::TraderEvent>(const trader::TraderEvent &event) {
  using namespace trader;
  switch (event) {
  case TraderEvent::Initialized:
    return "initialized";
  case TraderEvent::Connected:
    return "connected to the server";
  case TraderEvent::Disconnected:
    return "disconnected from the server";
  default:
    return std::format("unknown event {}", static_cast<uint8_t>(event));
  }
}
template <>
std::string toString<trader::LoginRequestEvent>(const trader::LoginRequestEvent &e) {
  return std::format("LoginRequestEvent {} {}", e.connectionId, utils::toString(e.request));
}

template <>
std::string toString<trader::LoginResponseEvent>(const trader::LoginResponseEvent &e) {
  return std::format("LoginResponseEvent {} {}", e.connectionId, utils::toString(e.response));
}
} // namespace utils
} // namespace hft

#endif // HFT_SERVER_TRADEREVENTS_HPP