/**
 * @author Vladimir Pavliv
 * @date 2025-03-30
 */

#ifndef HFT_COMMON_SYSTEMEVENTTRAITS_HPP
#define HFT_COMMON_SYSTEMEVENTTRAITS_HPP

namespace hft {

/**
 * @brief Allows targeted EventType subscriptions by key
 */
template <typename EventType>
struct SystemEventTraits {
  using KeyType = EventType;

  static KeyType getKey(const EventType &event) { return event; }
};

} // namespace hft

#endif // HFT_COMMON_SYSTEMEVENTTRAITS_HPP