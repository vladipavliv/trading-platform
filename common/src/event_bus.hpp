/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_EVENTBUS_HPP
#define HFT_COMMON_EVENTBUS_HPP

#include <functional>
#include <map>

#include "template_types.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Bus for synchronous message exchange
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
    getHandlers<EventType>().emplace_back(std::move(handler));
  }

  template <typename EventType>
  void publish(Span<EventType> event) {
    for (auto &handler : getHandlers<EventType>()) {
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