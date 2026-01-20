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
 * @brief Bus message compile-time blocker
 */
template <typename BusT, typename... MessageTs>
struct BusLimiter {
public:
  static_assert(Busable<BusT>, "BusT must satisfy the Busable concept");

  explicit BusLimiter(BusT &bus) : bus_{bus} {}

  template <typename T>
  static constexpr bool isAllowed() {
    return (std::is_same_v<T, MessageTs> || ...);
  }

  template <typename T>
    requires(isAllowed<T>())
  void post(const T &message) {
    bus_.post(message);
  }

private:
  BusT &bus_;
};
} // namespace hft

#endif // HFT_COMMON_BUSLIMITER_HPP
