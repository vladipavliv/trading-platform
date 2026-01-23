/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_CLIENT_EVENTS_HPP
#define HFT_CLIENT_EVENTS_HPP

#include <stdint.h>

#include "utils/string_utils.hpp"

namespace hft {
namespace client {

enum class Component : uint8_t {
  Engine = 1 << 0,
  Ipc = 1 << 1,
  Time = 1 << 2,
};

struct ComponentReady {
  Component id;
};

constexpr uint8_t INTERNAL_READY = (uint8_t)Component::Engine | (uint8_t)Component::Time;
constexpr uint8_t ALL_READY = INTERNAL_READY | (uint8_t)Component::Ipc;

enum class ServerConnectionState : uint8_t { Disconnected, Connected, Failed };
} // namespace client

inline String toString(const client::Component &cmp) {
  using namespace client;
  switch (cmp) {
  case Component::Engine:
    return "Engine";
  case Component::Ipc:
    return "Ipc";
  case Component::Time:
    return "Time";
  default:
    return "Unknown";
  }
}

inline String toString(const client::ServerConnectionState &event) {
  using namespace client;
  switch (event) {
  case ServerConnectionState::Disconnected:
    return "Disconnected from the server";
  case ServerConnectionState::Connected:
    return "Connected to the server";
  case ServerConnectionState::Failed:
    return "Failed to connect";
  default:
    return std::format("unknown event {}", static_cast<uint8_t>(event));
  }
}
} // namespace hft

#endif // HFT_CLIENT_EVENTS_HPP