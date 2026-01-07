/**
 * @author Vladimir Pavliv
 * @date 2026-01-07
 */

#ifndef HFT_COMMON_THREADUTILS_HPP
#define HFT_COMMON_THREADUTILS_HPP

#include <pthread.h>
#include <sched.h>
#include <stdexcept>
#include <system_error>

#include "logging.hpp"

namespace hft::utils {

inline void pinThreadToCore(int coreId) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(coreId, &cpuset);

  const int result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (result != 0) {
    LOG_ERROR("Failed to pin thread to core: {}, error: {}", coreId, result);
    throw std::system_error(result, std::generic_category(), "pthread_setaffinity_np");
  }
}

inline void setThreadRealTime(int priority = 99) {
  struct sched_param param;
  param.sched_priority = priority;
  const int result = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
  if (result != 0) {
    if (result == EPERM) {
      LOG_ERROR("Insufficient permissions for Real-Time priority (99). ");
      return;
    }
    LOG_ERROR("Failed to set real-time priority: {}, error: {}", priority, result);
    throw std::system_error(result, std::generic_category(), "pthread_setschedparam");
  }
}

} // namespace hft::utils

#endif // HFT_COMMON_THREADUTILS_HPP
