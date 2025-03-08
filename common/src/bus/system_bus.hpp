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
#include "template_types.hpp"
#include "types.hpp"
#include "utils/template_utils.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Bus for system commands and events
 * @details Offloads all the callbacks to a separate system io_context. Some system operations
 * like orders rerouting (to be released) might introduce slight delays, so to keep hot paths intact
 * its run in a single separate thread.
 * Allows both subscriptions for specific values with a callback, and generic subscriptions
 * to any events of a given type
 */
class SystemBus {
public:
  /**
   * @brief Exposing system io_context for any system tasks or timer needs across the system
   */
  IoContext systemIoCtx;

  SystemBus() : ioCtxGuard_{MakeGuard(systemIoCtx.get_executor())} {}

  template <typename EventType>
  void subscribe(CRefHandler<EventType> handler) {
    /**
     * Currently all system subscribers are static long living objects, but offloading all
     * operations to a system io ctx ensures synchronization since it runs single threaded
     */
    systemIoCtx.post([handler = std::move(handler)]() {
      getEventSubscribers<EventType>().emplace_back(std::move(handler));
    });
  }

  template <typename EventType>
  void subscribe(EventType event, Callback callback) {
    static_assert(utils::UnorderedMapKey<EventType> && "Type not suitable for value subscriptions");
    systemIoCtx.post([event, callback = std::move(callback)]() {
      getValueSubscribers<EventType>()[event].emplace_back(std::move(callback));
    });
  }

  template <typename EventType>
  void publish(CRef<EventType> event) {
    systemIoCtx.post([event]() {
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
    });
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

  ContextGuard ioCtxGuard_;
};

} // namespace hft

#endif // HFT_COMMON_SYSTEMBUS_HPP