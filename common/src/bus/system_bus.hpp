/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_SYSTEMBUS_HPP
#define HFT_COMMON_SYSTEMBUS_HPP

#include "boost_types.hpp"
#include "config/config.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Bus for system commands and events.
 * Allows subscribing to both event in general and specific values
 * @details All the callbacks are executed in a single-threaded system io context
 * It helpes to minimize effect on the hot path if system events happen there,
 * and avoids any microdelays as event handling is immediately offloaded to a separate thread
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
  inline void post(CRef<EventType> event) {
    ioCtx.post([event]() { onEvent(event); });
  }

  void run() {
    utils::setTheadRealTime();
    const auto coreId = Config::get_optional<size_t>("cpu.core_system");
    if (coreId.has_value()) {
      utils::pinThreadToCore(coreId.value());
    }
    ioCtx.run();
  }

  void stop() { ioCtx.stop(); }

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