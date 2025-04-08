/**
 * @author Vladimir Pavliv
 * @date 2025-03-30
 */

#ifndef HFT_COMMON_SYSTEMSAFEBUS_HPP
#define HFT_COMMON_SYSTEMSAFEBUS_HPP

#include <list>

#include "market_types.hpp"
#include "message_bus.hpp"
#include "template_types.hpp"
#include "types.hpp"

namespace hft {

/**
 * @brief Allows defining a key for the message type and subscribing via key value
 * e.g. LoginResponse message could be subscribed via the socket id
 */
template <typename EventType>
struct SystemEventTraits {
  using KeyType = EventType;

  static KeyType getKey(const EventType &event) { return event; }
};

/**
 * @brief SystemBusV2 with safe callbacks that allow unsubscribing
 * Design was slightly changed, and in current implementation all subscribers are long living
 * objects so unsub functionality is not really needed, and simpler SystemBus version is used
 */
class SystemSafeBus {
public:
  IoCtx ioCtx;

  SystemSafeBus() : guard_{MakeGuard(ioCtx.get_executor())} {}

  template <typename EventType>
  using SubscriptionHandler = CRefHandler<EventType>;
  template <typename EventType>
  using Subscription = WPtr<CRefHandler<EventType>>;

  /**
   * @brief Generic subscription to any event of EventType
   */
  template <typename EventType>
  void subscribe(ObjectId id, Subscription<EventType> handler) {
    ioCtx.post([this, id, handler = std::move(handler)]() {
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
    ioCtx.post([this, id, key, handler = std::move(handler)]() {
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
    ioCtx.post([this, id]() {
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
    ioCtx.post([event]() {
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
  SystemSafeBus(const SystemSafeBus &) = delete;
  SystemSafeBus(SystemSafeBus &&) = delete;
  SystemSafeBus &operator=(const SystemSafeBus &) = delete;
  SystemSafeBus &operator=(const SystemSafeBus &&) = delete;

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
  ContextGuard guard_;
};
} // namespace hft

#endif // HFT_COMMON_SYSTEMSAFEBUS_HPP
