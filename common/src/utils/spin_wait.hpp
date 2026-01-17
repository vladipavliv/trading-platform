/**
 * @author Vladimir Pavliv
 * @date 2026-01-13
 */

#ifndef HFT_COMMON_SPINWAIT_HPP
#define HFT_COMMON_SPINWAIT_HPP

#include <thread>

#include "constants.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"

namespace hft {

class SpinWait {
public:
  explicit SpinWait(size_t cap = SPIN_RETRIES_WARM) : cap_{cap} {}

  inline bool operator++() noexcept {
    if (cycles_ >= cap_) {
      return false;
    }
    ++cycles_;

    if (cycles_ < SPIN_RETRIES_HOT) {
      return true;
    } else if (cycles_ < SPIN_RETRIES_WARM) {
      asm volatile("pause" ::: "memory");
      return true;
    } else if (cycles_ < SPIN_RETRIES_YIELD) {
      std::this_thread::yield();
      return true;
    }
    return false;
  }

  inline void reset() noexcept { cycles_ = 0; }

  inline size_t cycles() noexcept { return cycles_; }

private:
  const size_t cap_{0};
  size_t cycles_{0};
};

} // namespace hft

#endif // HFT_COMMON_SPINWAIT_HPP