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
template <typename... Events>
class MessageBus {
public:
  MessageBus() : handlers_{} {}

  template <typename Event>
  static constexpr bool Routed = utils::contains<Event, Events...>;

  template <typename Event>
    requires Routed<Event>
  void subscribe(CRefHandler<Event> handler) {
    auto &handlerRef = std::get<CRefHandler<Event>>(handlers_);
    if (handlerRef) {
      LOG_ERROR("Handler is already registered for the type {}", typeid(Event).name());
    } else {
      handlerRef = std::move(handler);
    }
  }

  template <typename Event>
    requires Routed<Event>
  inline void post(CRef<Event> event) {
    auto &handlerRef = std::get<CRefHandler<Event>>(handlers_);
    if (!handlerRef) {
      LOG_ERROR("Handler not registered for event type");
      return;
    }
    handlerRef(event);
  }

  void run() {
    if (!(std::get<CRefHandler<Events>>(handlers_) && ...)) {
      throw std::runtime_error("Some handlers are not set in MessageBus");
    }
  }

  void stop() {}

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
