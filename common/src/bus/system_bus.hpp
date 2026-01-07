/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_SYSTEMBUS_HPP
#define HFT_COMMON_SYSTEMBUS_HPP

#include <map>

#include "bus_limiter.hpp"
#include "config/config.hpp"
#include "execution.hpp"
#include "functional_types.hpp"
#include "internal_error.hpp"
#include "primitive_types.hpp"
#include "utils/thread_utils.hpp"
#include "utils/trait_utils.hpp"

namespace hft {

/**
 * @brief Bus for system commands and events.
 * Allows subscribing to both event in general and specific values
 * @details All the callbacks are executed in a single-threaded system io context
 */
class SystemBus {
public:
  SystemBus() : ioCtxGuard_{MakeGuard(ioCtx_.get_executor())} {}

  inline IoCtx &systemIoCtx() { return ioCtx_; }

  template <typename EventType>
  void subscribe(CRefHandler<EventType> &&handler) {
    ioCtx_.post([handler = std::move(handler)]() mutable {
      getEventSubscribers<EventType>().emplace_back(std::move(handler));
    });
  }

  template <utils::UnorderedMapKey EventType>
  void subscribe(EventType event, Callback &&callback) {
    ioCtx_.post([event, callback = std::move(callback)]() mutable {
      getValueSubscribers<EventType>()[event].emplace_back(std::move(callback));
    });
  }

  template <typename EventType>
  inline void post(CRef<EventType> event) {
    ioCtx_.post([event]() { onEvent(event); });
  }

  void run() {
    utils::setThreadRealTime();
    const auto coreId = Config::get_optional<size_t>("cpu.core_system");
    if (coreId.has_value()) {
      utils::pinThreadToCore(coreId.value());
    }
    ioCtx_.run();
  }

  void stop() { ioCtx_.stop(); }

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
    if constexpr (utils::UnorderedMapKey<EventType>) {
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
  using ValueSubscribers = std::unordered_map<EventType, std::vector<Callback>>;

  template <typename EventType>
  static EventSubscribers<EventType> &getEventSubscribers() {
    static EventSubscribers<EventType> subscribers;
    static auto *fast_ptr = &subscribers;
    return *fast_ptr;
  }

  template <typename EventType>
  static ValueSubscribers<EventType> &getValueSubscribers() {
    static ValueSubscribers<EventType> subscribers;
    static auto *fast_ptr = &subscribers;
    return *fast_ptr;
  }

private:
  IoCtx ioCtx_;
  IoCtxGuard ioCtxGuard_;
};

using ErrorBus = BusLimiter<SystemBus, InternalError>;

} // namespace hft

#endif // HFT_COMMON_SYSTEMBUS_HPP