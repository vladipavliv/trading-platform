/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_SYSTEMBUS_HPP
#define HFT_COMMON_SYSTEMBUS_HPP

#include <functional>
#include <map>
#include <typeinfo>

#include "template_types.hpp"
#include "types.hpp"
#include "utils/template_utils.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Bus for system commands and events
 * @details Allows both subscriptions for specific values with a callback,
 * and generic subscriptions to event with a handler that wopuld be called with published event
 */
class SystemBus {
public:
  SystemBus() = default;

  template <typename EventType>
  void subscribe(CRefHandler<EventType> handler) {
    getEventSubscribers<EventType>().emplace_back(std::move(handler));
  }

  template <typename EventType>
  void subscribe(EventType event, Callback callback) {
    static_assert(utils::UnorderedMapKey<EventType> && "Type not suitable for value subscriptions");
    getValueSubscribers<EventType>()[event].emplace_back(std::move(callback));
  }

  template <typename EventType>
  void publish(CRef<EventType> event) {
    // First notify generic subscribers for EventType
    for (auto &handler : getEventSubscribers<EventType>()) {
      handler(event);
    }
    // Now try to notify value subscribers if possible
    if constexpr (utils::UnorderedMapKey<EventType>) {
      auto &valueSubscribers = getValueSubscribers<EventType>();
      if (valueSubscribers.count(event) > 0) {
        for (auto &callback : valueSubscribers[event]) {
          callback();
        }
      }
    }
  }

private:
  SystemBus(const SystemBus &) = delete;
  SystemBus(SystemBus &&) = delete;
  SystemBus &operator=(const SystemBus &) = delete;
  SystemBus &operator=(const SystemBus &&) = delete;

  template <typename EventType>
  using EventSubscribers = std::vector<CRefHandler<EventType>>;

  template <typename EventType>
  using ValueSubscribers = HashMap<EventType, std::vector<Callback>>;

  template <typename EventType>
  static EventSubscribers<EventType> &getEventSubscribers() {
    static EventSubscribers<EventType> subscribers;
    return subscribers;
  }

  template <typename EventType>
  static ValueSubscribers<EventType> &getValueSubscribers() {
    static ValueSubscribers<EventType> subscribers;
    return subscribers;
  }
};

} // namespace hft

#endif // HFT_COMMON_SYSTEMBUS_HPP