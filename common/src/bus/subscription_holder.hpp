/**
 * @author Vladimir Pavliv
 * @date 2025-03-30
 */

#ifndef HFT_COMMON_SUBSCRIPTIONHOLDER_HPP
#define HFT_COMMON_SUBSCRIPTIONHOLDER_HPP

#include <vector>

#include "bus/system_bus.hpp"
#include "template_types.hpp"
#include "types.hpp"

namespace hft {

/**
 * @brief Convenient holder for subscription callbacks
 * Erases the callback type and stores callbacks on the heap
 */
class SubscriptionHolder {
public:
  SubscriptionHolder(ObjectId id, SystemBus &bus) : subscriberId_{id}, bus_{bus} {
    callbacks.reserve(10);
  }

  ~SubscriptionHolder() { /* bus_.unsubscribe(subscriberId_); */ }

  template <typename HandlerType>
  WPtr<HandlerType> add(HandlerType &&callback) {
    auto sharedClb = std::make_shared<HandlerType>(std::forward<HandlerType>(callback));
    callbacks.emplace_back(sharedClb);
    return sharedClb;
  }

private:
  ObjectId subscriberId_;
  SystemBus &bus_;
  std::vector<SPtr<void>> callbacks;
};

} // namespace hft

#endif // HFT_COMMON_SUBSCRIPTIONHOLDER_HPP