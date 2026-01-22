/**
 * @author Vladimir Pavliv
 * @date 2026-01-22
 */

#ifndef HFT_COMMON_FUTEXWAITER_HPP
#define HFT_COMMON_FUTEXWAITER_HPP

#include "logging.hpp"
#include "primitive_types.hpp"
#include "utils/sync_utils.hpp"

namespace hft {

struct ALIGN_CL FutexWaiter {
  AtomicUInt32 futex{0};      // futex
  AtomicBool waitFlag{false}; // optimization to hit futex only when necessary

  uint32_t prepareWait() {
    const auto val = futex.load(std::memory_order_acquire);
    waitFlag.store(true, std::memory_order_release);
    return val;
  }

  void cancelWait() { waitFlag.store(false, std::memory_order_release); }

  void wait(uint32_t val) {
    utils::futexWait(futex, val);
    waitFlag.store(false, std::memory_order_release);
  }

  void notify() {
    if (waitFlag.load(std::memory_order_acquire)) {
      futex.fetch_add(1, std::memory_order_release);
      utils::futexWake(futex);
    }
  }
};
} // namespace hft
#endif // HFT_COMMON_FUTEXWAITER_HPP