/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_SYSTEMBUS_HPP
#define HFT_COMMON_SYSTEMBUS_HPP

#include <functional>
#include <map>
#include <typeinfo>

#include "boost_types.hpp"

#include "types.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Bus for system commands and events.
 * Allows subscribing to both event in general and specific values
 * @details All the callbacks are executed in a system io context
 */
class SystemBus {
public:
  IoCtx ioCtx;

  SystemBus() : ioCtxGuard_{MakeGuard(ioCtx.get_executor())} {}

  template <typename EventType>
  void subscribe(CRefHandler<EventType> handler) {
    ioCtx.post([handler = std::move(handler)]() {
      getEventSubscribers<EventType>().emplace_back(std::move(handler));
    });
  }

  template <UnorderedMapKey EventType>
  void subscribe(EventType event, Callback callback) {
    ioCtx.post([event, callback = std::move(callback)]() {
      getValueSubscribers<EventType>()[event].emplace_back(std::move(callback));
    });
  }

  template <typename EventType>
  void post(CRef<EventType> event) {
    ioCtx.post([event]() { onEvent(event); });
  }

private:
  SystemBus(const SystemBus &) = delete;
  SystemBus(SystemBus &&) = delete;
  SystemBus &operator=(const SystemBus &) = delete;
  SystemBus &operator=(const SystemBus &&) = delete;

  template <typename EventType>
  static void onEvent(CRef<EventType> event) {
    // First notify generic subscribers for EventType
    for (auto &handler : getEventSubscribers<EventType>()) {
      handler(event);
    }
    // Now try to notify value subscribers if possible
    if constexpr (UnorderedMapKey<EventType>) {
      auto &valueSubscribers = getValueSubscribers<EventType>();
      if (valueSubscribers.count(event) > 0) {
        for (auto &callback : valueSubscribers[event]) {
          callback();
        }
      }
    }
  }

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

private:
  ContextGuard ioCtxGuard_;
};

} // namespace hft

#endif // HFT_COMMON_SYSTEMBUS_HPP