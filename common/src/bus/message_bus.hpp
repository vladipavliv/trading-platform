/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_MESSAGEBUS_HPP
#define HFT_COMMON_MESSAGEBUS_HPP

#include <functional>
#include <tuple>
#include <type_traits>

#include "logging.hpp"
#include "primitive_types.hpp"
#include "ptr_types.hpp"

namespace hft {

/**
 * @brief Message bus for fast direct message flow
 * @details For better cache locality all types are defined via variadics
 * And only one consumer is allowed per message type
 */
template <typename... Events>
class MessageBus {
public:
  MessageBus() : handlers_{} {}

  template <typename Event>
  static constexpr bool Routed = (std::is_same_v<Event, Events> || ...);

  template <typename Event>
    requires Routed<Event>
  void subscribe(CRefHandler<Event> &&handler) {
    auto &handlerRef = std::get<CRefHandler<Event>>(handlers_);
    handlerRef = std::move(handler);
  }

  template <typename Event>
    requires Routed<Event>
  inline void post(CRef<Event> event) {
    auto &handlerRef = std::get<CRefHandler<Event>>(handlers_);
    assert(handlerRef);
    handlerRef(event);
  }

private:
  MessageBus(const MessageBus &) = delete;
  MessageBus(MessageBus &&) = delete;
  MessageBus &operator=(const MessageBus &) = delete;
  MessageBus &operator=(const MessageBus &&) = delete;

private:
  std::tuple<CRefHandler<Events>...> handlers_;
};

} // namespace hft

#endif // HFT_COMMON_MESSAGEBUS_HPP
