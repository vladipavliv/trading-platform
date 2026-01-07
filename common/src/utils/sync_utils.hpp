/**
 * @author Vladimir Pavliv
 * @date 2026-01-07
 */

#ifndef HFT_COMMON_SYNCUTILS_HPP
#define HFT_COMMON_SYNCUTILS_HPP

#include <atomic>
#include <cstdint>
#include <ctime>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "logging.hpp"

namespace hft::utils {

using Futex = std::atomic<uint32_t>;

inline void futexWake(Futex &futex, int count = 1) {
  syscall(SYS_futex, reinterpret_cast<uint32_t *>(&futex), FUTEX_WAKE, count, nullptr, nullptr, 0);
}

inline void futexWait(Futex &futex, uint32_t curr, uint32_t timeout = 0) {
  if (timeout == 0) {
    LOG_DEBUG("futexWait");
    syscall(SYS_futex, reinterpret_cast<uint32_t *>(&futex), FUTEX_WAIT, curr, nullptr, nullptr, 0);
  } else {
    LOG_DEBUG("futexWait timed {}", timeout);
    struct timespec ts {
      .tv_sec = static_cast<time_t>(timeout / 1000),
      .tv_nsec = static_cast<long>((timeout % 1000) * 1000000)
    };
    syscall(SYS_futex, reinterpret_cast<uint32_t *>(&futex), FUTEX_WAIT, curr, &ts, nullptr, 0);
  }
}

inline void hybridWait(Futex &futex, uint32_t curr, uint64_t spinCount = 10'000) {
  LOG_DEBUG("hybridWait");
  for (uint64_t i = 0; i < spinCount; ++i) {
    if (futex.load(std::memory_order_acquire) != curr)
      return;
    __builtin_ia32_pause();
  }
  if (futex.load(std::memory_order_acquire) == curr) {
    futexWait(futex, curr);
  }
}

} // namespace hft::utils

#endif // HFT_COMMON_SYNCUTILS_HPP
