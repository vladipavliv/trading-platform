/**
 * @author Vladimir Pavliv
 * @date 2026-01-07
 */

#ifndef HFT_COMMON_IDUTILS_HPP
#define HFT_COMMON_IDUTILS_HPP

#include <atomic>

namespace hft::utils {

inline auto generateOrderId() -> uint64_t {
  static std::atomic<uint64_t> counter{0};
  return counter.fetch_add(1, std::memory_order_relaxed);
}

inline auto generateConnectionId() -> uint64_t {
  static std::atomic<uint64_t> counter{0};
  return counter.fetch_add(1, std::memory_order_relaxed);
}

inline auto generateToken() -> uint64_t {
  static std::atomic<uint64_t> counter{0};
  return counter.fetch_add(1, std::memory_order_relaxed);
}

} // namespace hft::utils

#endif // HFT_COMMON_IDUTILS_HPP
