/**
 * @author Vladimir Pavliv
 * @date 2026-01-09
 */

#ifndef HFT_COMMON_PROFILINGUTILS_HPP
#define HFT_COMMON_PROFILINGUTILS_HPP

#include <stdint.h>
#include <sys/resource.h>
#include <system_error>
#include <x86intrin.h>

#include "metadata_types.hpp"

namespace hft::utils {

#ifdef PROFILING
inline auto getRUsage() -> RUsageSnapshot {
  struct rusage usage {};
  if (getrusage(RUSAGE_THREAD, &usage) != 0) {
    throw std::system_error(errno, std::generic_category(), "getrusage failed");
  }

  RUsageSnapshot snap{};
  snap.volSwitches = usage.ru_nvcsw;
  snap.involSwitches = usage.ru_nivcsw;

  snap.userTimeNs = usage.ru_utime.tv_sec * 1'000'000'000ull + usage.ru_utime.tv_usec * 1'000ull;
  snap.systemTimeNs = usage.ru_stime.tv_sec * 1'000'000'000ull + usage.ru_stime.tv_usec * 1'000ull;

  snap.softPageFaults = usage.ru_minflt;
  snap.hardPageFaults = usage.ru_majflt;

  return snap;
}

#define PROFILE_EXEC(expr, counter)                                                                \
  ([&]() {                                                                                         \
    unsigned aux;                                                                                  \
    const uint64_t start = __rdtscp(&aux);                                                         \
    auto result = (expr);                                                                          \
    const uint64_t delta = __rdtscp(&aux) - start;                                                 \
    counter = std::max(counter, delta);                                                            \
    return result;                                                                                 \
  }())

#define GET_RUSAGE() utils::getRUsage()
#else
#define PROFILE_EXEC(f, counter) (f)
#define GET_RUSAGE() (RUsageSnapshot{})
#endif

} // namespace hft::utils

#endif // HFT_COMMON_PROFILINGUTILS_HPP
