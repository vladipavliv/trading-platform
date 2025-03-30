/**
 * @author Vladimir Pavliv
 * @date 2025-03-30
 */

#ifndef HFT_COMMON_SYSTEMBUS_HPP
#define HFT_COMMON_SYSTEMBUS_HPP

#include <list>

#include "io_ctx.hpp"
#include "market_types.hpp"
#include "message_bus.hpp"
#include "system_event_traits.hpp"
#include "template_types.hpp"
#include "types.hpp"

namespace hft {

/**
 * @brief Safe system event pub/sub,
 * @details subscriber holds shared pointer to a callback
 * to track its destruction. Tracking weak pointers to subscriber itself would mean
 * subscribers should be managed by shared_ptr, and thats a big restriction.
 * @details Upon subscribing saves subscription iterator, wraps it into a callback
 * for quick unsubscribe, and stores in subscribers map
 */
class SystemBus {
public:
  IoCtx ioCtx;

  SystemBus() {}

  template <typename EventType>
  using SubscriptionHandler = CRefHandler<EventType>;
  template <typename EventType>
  using Subscription = WPtr<CRefHandler<EventType>>;

  /**
   * @brief Generic subscription to any event of EventType
   */
  template <typename EventType>
  void subscribe(ObjectId id, Subscription<EventType> handler) {
    ioCtx.ctx.post([this, id, handler = std::move(handler)]() {
      auto &subs = getTypeSubs<EventType>();
      subs.emplace_back(handler);
      auto iterator = std::prev(subs.end(), 1);
      // Add unsub callback
      subscribers_[id].emplace_back([&subs, iterator]() { subs.erase(iterator); });
    });
  }

  /**
   * @brief Subscription to value of EventType, defined via SystemEventTraits
   * @details For simple types like enums uses event value itself, for struct types
   * allows to define mapping of event on a key
   */
  template <typename EventType>
  void subscribe(ObjectId id, SystemEventTraits<EventType>::KeyType key,
                 Subscription<EventType> handler) {
    ioCtx.ctx.post([this, id, key, handler = std::move(handler)]() {
      auto &subsMap = getValueSubs<EventType>();
      auto &subs = subsMap[key];
      subs.emplace_back(handler);
      auto iterator = std::prev(subs.end(), 1);
      // Add unsub callback
      subscribers_[id].emplace_back([&subs, &subsMap, key, iterator]() {
        subs.erase(iterator);
        if (subs.empty()) {
          subsMap.erase(key);
        }
      });
    });
  }

  void unsubscribe(ObjectId id) {
    ioCtx.ctx.post([this, id]() {
      if (subscribers_.count(id) == 0) {
        return;
      }
      for (auto &unsub : subscribers_[id]) {
        unsub();
      }
      subscribers_.erase(id);
    });
  }

  template <typename EventType>
  void post(CRef<EventType> event) {
    ioCtx.ctx.post([event]() {
      // First notify generic type subscribers
      auto &typeSubs = getTypeSubs<EventType>();
      for (auto &weakHandler : typeSubs) {
        auto sharedHandler = weakHandler.lock();
        if (sharedHandler != nullptr) {
          (*sharedHandler)(event);
        }
      }
      // Then try to notify value subscribers if event is suitable
      if constexpr (UnorderedMapKey<typename SystemEventTraits<EventType>::KeyType>) {
        auto &valueSubsMap = getValueSubs<EventType>();
        auto key = SystemEventTraits<EventType>::getKey(event);
        if (valueSubsMap.count(key) > 0) {
          for (auto &weakHandler : valueSubsMap[key]) {
            auto sharedHandler = weakHandler.lock();
            if (sharedHandler != nullptr) {
              (*sharedHandler)(event);
            }
          }
        }
      }
    });
  }

private:
  SystemBus(const SystemBus &) = delete;
  SystemBus(SystemBus &&) = delete;
  SystemBus &operator=(const SystemBus &) = delete;
  SystemBus &operator=(const SystemBus &&) = delete;

private:
  /**
   * @brief Map of subscriber id on a list of unsubscribe callbacks
   */
  using Subscribers = HashMap<ObjectId, std::list<Callback>>;

  template <typename EventType>
  using ValueSubsList = std::list<Subscription<EventType>>;
  template <typename EventType>
  using ValueSubsIterator = ValueSubsList<EventType>::iterator;

  template <typename EventType>
  using ValueSubs =
      HashMap<typename SystemEventTraits<EventType>::KeyType, ValueSubsList<EventType>>;

  template <typename EventType>
  using TypeSubsList = std::list<Subscription<EventType>>;
  template <typename EventType>
  using TypeSubsIterator = ValueSubsList<EventType>::iterator;

  template <typename EventType>
  static ValueSubs<EventType> &getValueSubs() {
    static ValueSubs<EventType> subs;
    return subs;
  }

  template <typename EventType>
  static TypeSubsList<EventType> &getTypeSubs() {
    static TypeSubsList<EventType> subs;
    return subs;
  }

  Subscribers subscribers_;
};
} // namespace hft

#endif // HFT_COMMON_SYSTEMBUS_HPP
