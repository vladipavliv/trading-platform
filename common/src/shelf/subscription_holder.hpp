/**
 * @author Vladimir Pavliv
 * @date 2025-03-30
 */

#ifndef HFT_COMMON_SUBSCRIPTIONHOLDER_HPP
#define HFT_COMMON_SUBSCRIPTIONHOLDER_HPP

#include <vector>

#include "bus/system_bus.hpp"

#include "types.hpp"

namespace hft {

/**
 * @brief Convenient holder for subscription callbacks
 * Using weak pointers to a subscriber itself would work only if its managed by the shared_ptr
 * Which is a significant limitation, so instead subscription callbacks are managed by shared_ptr
 * @details Currently not used but maybe would be usefull later
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