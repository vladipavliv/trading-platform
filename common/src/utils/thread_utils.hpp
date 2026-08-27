/**
 * @author Vladimir Pavliv
 * @date 2026-01-07
 */

#ifndef HFT_COMMON_THREADUTILS_HPP
#define HFT_COMMON_THREADUTILS_HPP

#include <cstdint>
#include <format>
#include <pthread.h>
#include <sched.h>
#include <system_error>
#include <x86intrin.h>

#include "logging.hpp"

namespace hft::utils {

constexpr uint32_t CORE_ID_MASK = 0xFFF;

inline void pinThreadToCore(int coreId) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(coreId, &cpuset);

  const int result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (result != 0) {
    LOG_ERROR_SYSTEM("Failed to pin thread to core: {}, error: {}", coreId, result);
  }
}

inline void setThreadRealTime(int priority = 99) {
  struct sched_param param = {.sched_priority = priority};
  int result = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
  if (result != 0) {
#ifndef CICD
    throw std::system_error(result, std::generic_category(),
                            std::format("SCHED_FIFO failed, run as root or check rlimits"));
#else
    LOG_ERROR_SYSTEM("SCHED_FIFO failed");
#endif
  }
}

[[nodiscard]] inline __attribute__((always_inline)) auto getCoreId() -> uint32_t {
  unsigned cpu, node;
  if (getcpu(&cpu, &node) == 0)
    return cpu;
  return -1;
}

inline void join(std::jthread &th) {
  if (th.joinable() && std::this_thread::get_id() != th.get_id()) {
    th.join();
  }
}

} // namespace hft::utils

#endif // HFT_COMMON_THREADUTILS_HPP
