/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_SERVEREVENTS_HPP
#define HFT_SERVER_SERVEREVENTS_HPP

#include "functional_types.hpp"
#include "primitive_types.hpp"
#include "status_code.hpp"
#include "transport/connection_status.hpp"
#include "utils/string_utils.hpp"

namespace hft {
namespace server {

enum class Component : uint8_t {
  Coordinator = 1 << 0,
  Gateway = 1 << 1,
  Ipc = 1 << 2,
  Time = 1 << 3,
};

struct ComponentReady {
  Component id;
};

constexpr uint8_t INTERNAL_READY =
    (uint8_t)Component::Coordinator | (uint8_t)Component::Gateway | (uint8_t)Component::Time;

constexpr uint8_t ALL_READY = INTERNAL_READY | (uint8_t)Component::Ipc;

// TODO
struct ChannelStatusEvent {
  Optional<ClientId> clientId;
  ConnectionStatusEvent event;
};
} // namespace server

inline String toString(const server::Component &cmp) {
  using namespace server;
  switch (cmp) {
  case Component::Coordinator:
    return "Coordinator";
  case Component::Gateway:
    return "Gateway";
  case Component::Ipc:
    return "Ipc";
  case Component::Time:
    return "Time";
  default:
    return "Unknown";
  }
}

inline String toString(const server::ChannelStatusEvent &event) {
  using namespace server;
  return std::format("ChannelStatusEvent {} {}", event.clientId.value_or(0), toString(event.event));
}
} // namespace hft

#endif // HFT_SERVER_SERVEREVENTS_HPP