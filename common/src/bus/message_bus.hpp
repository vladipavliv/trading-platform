/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_MESSAGEBUS_HPP
#define HFT_COMMON_MESSAGEBUS_HPP

#include <functional>
#include <map>
#include <typeinfo>

#include "logging.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Message bus for fast direct message flow
 * @details For better cache locality all types are defined via variadics
 * And only one consumer is allowed per message type
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
    if (handlerRef) {
      LOG_ERROR("Handler is already registered for the type {}", typeid(EventType).name());
    } else {
      handlerRef = std::move(handler);
    }
  }

  template <typename EventType>
    requires RoutedType<EventType>
  inline void post(CRef<EventType> event) {
    auto &handlerRef = std::get<CRefHandler<EventType>>(handlers_);
    if (!handlerRef) {
      LOG_ERROR("Handler not registered for event type");
      return;
    }
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
