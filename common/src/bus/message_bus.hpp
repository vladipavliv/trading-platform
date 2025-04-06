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
#include "utils/template_utils.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Message bus for fast, direct message flow.
 * @details For better cache locality all types are defined
 * via variadics and only one message consumer is allowed pr message type
 * a container type
 */
template <typename... EventTypes>
class MessageBus {
public:
  MessageBus() : handlers_{std::make_tuple(CRefHandler<EventTypes>{}...)} {}

  template <typename EventType>
  static constexpr bool RoutedType = utils::contains<EventType, EventTypes...>;

  template <typename EventType>
    requires RoutedType<EventType>
  void setHandler(CRefHandler<EventType> handler) {
    auto &handlerRef = std::get<CRefHandler<EventType>>(handlers_);
    if (handlerRef != nullptr) {
      LOG_ERROR("Handler is already registered for the type {}", typeid(EventType).name());
    } else {
      handlerRef = std::move(handler);
    }
  }

  template <typename EventType>
    requires RoutedType<EventType>
  void post(CRef<EventType> event) {
    auto &handlerRef = std::get<CRefHandler<EventType>>(handlers_);
    assert(handlerRef != nullptr && "Handler not registered for event type");
    handlerRef(event);
  }

private:
  MessageBus(const MessageBus &) = delete;
  MessageBus(MessageBus &&) = delete;
  MessageBus &operator=(const MessageBus &) = delete;
  MessageBus &operator=(const MessageBus &&) = delete;

private:
  std::tuple<CRefHandler<EventTypes>...> handlers_;
};

} // namespace hft

#endif // HFT_COMMON_MESSAGEBUS_HPP