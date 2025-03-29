/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_MESSAGEBUS_HPP
#define HFT_COMMON_MESSAGEBUS_HPP

#include <functional>
#include <map>
#include <typeinfo>

#include "template_types.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Message bus for fast, direct message flow between objects
 * @details Designed for hot market data paths, with only one consumer for a given message type
 * Message types are predefined to keep all handlers close together and cache friendly
 * Passes messages around via std::span, its lightweight enough to not introduce
 * much performance impact. Single values are wrapped in a Span(&value, 1) with additional
 * indirection but the value is most likely on the stack so the impact is also negligible.
 */
template <typename... EventTypes>
class MessageBus {
public:
  MessageBus() : spanHandlers_{std::make_tuple(SpanHandler<EventTypes>{}...)} {}

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
  void post(Span<EventType> event) {
    auto &handlerRef = std::get<SpanHandler<EventType>>(spanHandlers_);
    assert(handlerRef != nullptr && "Handler not registered for event type");
    handlerRef(event);
  }

private:
  MessageBus(const MessageBus &) = delete;
  MessageBus(MessageBus &&) = delete;
  MessageBus &operator=(const MessageBus &) = delete;
  MessageBus &operator=(const MessageBus &&) = delete;

private:
  std::tuple<SpanHandler<EventTypes>...> spanHandlers_;
};

} // namespace hft

#endif // HFT_COMMON_MESSAGEBUS_HPP