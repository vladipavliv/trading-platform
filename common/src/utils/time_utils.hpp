/**
 * @author Vladimir Pavliv
 * @date 2026-01-07
 */

#ifndef HFT_COMMON_TIMEUTILS_HPP
#define HFT_COMMON_TIMEUTILS_HPP

#include <chrono>
#include <cstdint>
#include <ctime>
#include <thread>
#include <utility>
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

inline __attribute__((always_inline)) auto getCycles() -> uint64_t {
  unsigned aux;
  return __rdtscp(&aux);
}

/**
 * @brief Returns multiplier to convert cpu cycles to ns
 */
[[nodiscard]] inline auto getNsPerCycle() -> double {
  for (int i = 0; i < 10; ++i) {
    getCycles();
    __asm__ volatile("" : : : "memory");
  }

  auto t1 = std::chrono::steady_clock::now();
  uint64_t c1 = getCycles();

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  uint64_t c2 = getCycles();
  auto t2 = std::chrono::steady_clock::now();

  auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();

  return static_cast<double>(ns) / static_cast<double>(c2 - c1);
}

} // namespace hft::utils

#endif // HFT_COMMON_TIMEUTILS_HPP
