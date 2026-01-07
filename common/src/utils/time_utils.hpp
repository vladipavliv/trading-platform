/**
 * @author Vladimir Pavliv
 * @date 2026-01-07
 */

#ifndef HFT_COMMON_TIMEUTILS_HPP
#define HFT_COMMON_TIMEUTILS_HPP

#include <cstdint>
#include <ctime>
#include <x86intrin.h>

namespace hft::utils {

[[nodiscard]] inline __attribute__((always_inline)) auto getTimestamp() -> uint64_t {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<uint64_t>(ts.tv_sec) * 1'000'000 + ts.tv_nsec / 1'000;
}

[[nodiscard]] inline __attribute__((always_inline)) auto getTimestampNs() -> uint64_t {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000 + ts.tv_nsec;
}

[[nodiscard]] inline __attribute__((always_inline)) auto getCycles() -> uint64_t {
  _mm_lfence();
  return __rdtsc();
}

} // namespace hft::utils

#endif // HFT_COMMON_TIMEUTILS_HPP
