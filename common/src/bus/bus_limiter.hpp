/**
 * @author Vladimir Pavliv
 * @date 2025-12-24
 */

#ifndef HFT_COMMON_BUSLIMITER_HPP
#define HFT_COMMON_BUSLIMITER_HPP

#include <concepts>

#include "bus/busable.hpp"

namespace hft {

/**
 * @brief Limits message types that could be posted to the bus
 */
template <Busable Bus, typename... MessageTypes>
struct BusLimiter {
public:
  explicit BusLimiter(Bus &bus) : bus_{bus} {}

  template <typename T>
  static constexpr bool isAllowed() {
    return (std::is_same_v<T, MessageTypes> || ...);
  }

  template <typename T>
    requires(isAllowed<T>())
  void post(const T &message) {
    bus_.post(message);
  }

private:
  Bus &bus_;
};
} // namespace hft

#endif // HFT_COMMON_BUSLIMITER_HPP
