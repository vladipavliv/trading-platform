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
 * @details Simpler implementation without tuple so any object can access bus via static singleton
 * slightly less cache friendly then strict type management with tuple, but slightly more convenient
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
    spdlog::debug("Registered handler for {}", typeid(EventType).name());
    getHandlers<EventType>().emplace_back(std::move(handler));
  }

  template <typename EventType>
  void publish(Span<EventType> event) {
    auto &handlers = getHandlers<EventType>();
    if (handlers.empty()) {
      spdlog::error("No handlers registered for {}", typeid(EventType).name());
      return;
    }
    for (auto &handler : handlers) {
      handler(event);
    }
  }

  template <typename EventType>
  void publish(EventType event) {
    // extra pointer indirection wont cause much performance hit if object is in L1
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