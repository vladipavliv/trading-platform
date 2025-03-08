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
 * @details Executes all the callbacks in a system io context to not load the hot path
 * Allows both subscriptions for specific values with a callback, and generic
 * subscriptions to any events of a given type
 */
class SystemBus {
public:
  IoContext systemIoCtx;

  SystemBus() : ioCtxGuard_{boost::asio::make_work_guard(systemIoCtx)} {}

  template <typename EventType>
  void subscribe(CRefHandler<EventType> handler) {
    /**
     * Currently all subscribers are static long living objects. However offloading subscriptions to
     * a system io ctx ensures synchronization. Since systemn io ctx runs single-threaded -
     * subscribing and publishing won't go in parallel
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