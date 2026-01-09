/**
 * @author Vladimir Pavliv
 * @date 2026-01-07
 */

#ifndef HFT_COMMON_IDUTILS_HPP
#define HFT_COMMON_IDUTILS_HPP

#include <atomic>

namespace hft::utils {

inline auto generateOrderId() -> uint64_t {
  static uint64_t counter{0};
  return counter++;
}

inline auto generateConnectionId() -> uint64_t {
  static uint64_t counter{0};
  return counter++;
}

inline auto generateToken() -> uint64_t {
  static uint64_t counter{0};
  return counter++;
}

} // namespace hft::utils

#endif // HFT_COMMON_IDUTILS_HPP
