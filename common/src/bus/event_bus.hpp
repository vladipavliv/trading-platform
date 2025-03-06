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
 * @details To keep the main event path hot only one handler is allowed for each event type
 * and all handlers are kept in a tuple. All event types are defined beforehand
 * @todo Try captureless function pointers instead of std::function
 */
template <typename... EventTypes>
class EventBus {
public:
  EventBus()
      : crefHandlers_{std::make_tuple(CRefHandler<EventTypes>{}...)},
        spanHandlers_{std::make_tuple(SpanHandler<EventTypes>{}...)} {}

  template <typename EventType>
  void setHandler(CRefHandler<EventType> handler) {
    auto &handlerRef = std::get<CRefHandler<EventType>>(crefHandlers_);
    if (handlerRef != nullptr) {
      spdlog::error("CrefHandler is already registered for the type {}",
                    []() { return typeid(EventType).name(); }());
    } else {
      handlerRef = std::move(handler);
    }
  }

  template <typename EventType>
  void setHandler(SpanHandler<EventType> handler) {
    auto &handlerRef = std::get<SpanHandler<EventType>>(spanHandlers_);
    if (handlerRef != nullptr) {
      spdlog::error("SpanHandler is already registered for the type {}",
                    []() { return typeid(EventType).name(); }());
    } else {
      handlerRef = std::move(handler);
    }
  }

  template <typename EventType>
  void publish(CRef<EventType> event) {
    auto &handlerRef = std::get<CRefHandler<EventType>>(crefHandlers_);
    assert(handlerRef != nullptr && "Handler not registered for event type");
    handlerRef(event);
  }

  template <typename EventType>
  void publish(Span<EventType> event) {
    auto &handlerRef = std::get<SpanHandler<EventType>>(spanHandlers_);
    assert(handlerRef != nullptr && "Handler not registered for event type");
    handlerRef(event);
  }

private:
  EventBus(const EventBus &) = delete;
  EventBus(EventBus &&) = delete;
  EventBus &operator=(const EventBus &) = delete;
  EventBus &operator=(const EventBus &&) = delete;

private:
  std::tuple<CRefHandler<EventTypes>...> crefHandlers_;
  std::tuple<SpanHandler<EventTypes>...> spanHandlers_;
};

} // namespace hft

#endif // HFT_COMMON_EVENTBUS_HPP