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

#include "constants.hpp"
#include "logging.hpp"

namespace hft::utils {

using Futex = std::atomic<uint32_t>;
using FutexFlag = std::atomic<bool>;

inline void futexWake(Futex &futex, int count = 1) {
  syscall(SYS_futex, reinterpret_cast<uint32_t *>(&futex), FUTEX_WAKE, count, nullptr, nullptr, 0);
}

inline void futexWait(Futex &futex, uint32_t curr, uint64_t timeoutNs = 0) {
  if (timeoutNs == 0) {
    syscall(SYS_futex, reinterpret_cast<int32_t *>(&futex), FUTEX_WAIT, curr, nullptr, nullptr, 0);
  } else {
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(timeoutNs / 1000000000ULL);
    ts.tv_nsec = static_cast<long>(timeoutNs % 1000000000ULL);

    syscall(SYS_futex, reinterpret_cast<int32_t *>(&futex), FUTEX_WAIT, curr, &ts, nullptr, 0);
  }
}

inline uint32_t hybridWait(Futex &ftx, uint32_t val, FutexFlag &waitFlag, FutexFlag &stopFlag) {
  LOG_TRACE("hybridWait {}", val);
  for (uint32_t i = 0; i < BUSY_WAIT_CYCLES; ++i) {
    if (ftx.load(std::memory_order_acquire) != val || !stopFlag.load(std::memory_order_acquire))
      return i;
    __builtin_ia32_pause();
  }
  waitFlag.store(true, std::memory_order_release);
  futexWait(ftx, val);
  waitFlag.store(false, std::memory_order_release);
  return BUSY_WAIT_CYCLES;
}

} // namespace hft::utils

#endif // HFT_COMMON_SYNCUTILS_HPP
