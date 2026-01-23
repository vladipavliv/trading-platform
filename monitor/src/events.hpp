/**
 * @author Vladimir Pavliv
 * @date 2026-01-23
 */

#ifndef HFT_MONITOR_EVENTS_HPP
#define HFT_MONITOR_EVENTS_HPP

#include "primitive_types.hpp"

namespace hft {
namespace monitor {
enum class Component : uint8_t {
  Ipc = 1 << 0,
  Time = 1 << 1,
};

struct ComponentReady {
  Component id;
};

constexpr uint8_t INTERNAL_READY = (uint8_t)Component::Time;
constexpr uint8_t ALL_READY = INTERNAL_READY | (uint8_t)Component::Ipc;
} // namespace monitor

inline String toString(const monitor::Component &cmp) {
  using namespace monitor;
  switch (cmp) {
  case Component::Ipc:
    return "Ipc";
  case Component::Time:
    return "Time";
  default:
    return "Unknown";
  }
}
} // namespace hft

#endif // HFT_MONITOR_EVENTS_HPP