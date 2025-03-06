/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_EVENTBUS_HPP
#define HFT_COMMON_EVENTBUS_HPP

#include <functional>
#include <map>
#include <typeinfo>

#include "template_types.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Event bus to establish direct event flow between objects
 * @details By default events are passed via a span, its lightweight enough to not cause any
 * performance hits for either single value, or a container.
 * For a single value there might be an extra indirection, but if an object is located on the stack
 * and is still in a hot cache, that indirection cost would be negligible
 * Simpler implementation without tuple so any object can access bus via static singleton
 * Slightly less cache friendly for different types of events then the strict type management with
 * tuple, but slightly simpler and more convenient
 * @todo Try captureless function pointers instead of std::function
 */
class EventBus {
public:
  static EventBus &bus() {
    static EventBus instance;
    return instance;
  }

  template <typename EventType>
  void subscribe(SpanHandler<EventType> handler) {
    spdlog::debug("Registered handler for {}", []() { return typeid(EventType).name(); }());
    getHandlers<EventType>().emplace_back(std::move(handler));
  }

  template <typename EventType>
  void publish(Span<EventType> event) {
    auto &handlers = getHandlers<EventType>();
    spdlog::debug(
        "Publishing {} {}", []() { return typeid(EventType).name(); }(),
        [&handlers]() { return handlers.empty() ? "no handlers registered" : ""; }());
    for (auto &handler : handlers) {
      handler(event);
    }
  }

  template <typename EventType>
  void publish(EventType event) {
    publish(Span<EventType>{&event, 1});
  }

private:
  EventBus() = default;

  EventBus(const EventBus &) = delete;
  EventBus(EventBus &&) = delete;
  EventBus &operator=(const EventBus &) = delete;
  EventBus &operator=(const EventBus &&) = delete;

  template <typename EventType>
  static inline std::vector<SpanHandler<EventType>> &getHandlers() {
    static std::vector<SpanHandler<EventType>> handlers;
    return handlers;
  }
};

} // namespace hft

#endif // HFT_COMMON_EVENTBUS_HPP