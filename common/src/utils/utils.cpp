/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include "utils.hpp"

#include <functional>
#include <iostream>

#include <spdlog/spdlog.h>

#include <ctime>

namespace hft::utils {

void pinThreadToCore(int core_id) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core_id, &cpuset);

  pthread_t thread = pthread_self();
  int result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
  if (result != 0) {
    spdlog::error("Failed to set thread affinity");
  }
}

void setTheadRealTime() {
  // Set CPU affinity for the thread
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(0, &cpuset); // Bind to CPU core 0

  int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (rc != 0) {
    spdlog::error("Failed to set CPU affinity: {}", rc);
    return;
  }

  // Set real-time priority (SCHED_FIFO)
  struct sched_param param;
  param.sched_priority = 99; // Highest priority for real-time threads
  rc = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
  if (rc != 0) {
    spdlog::error("Failed to set real-time priority: {}", rc);
  }
}

size_t getTraderId(const TcpSocket &sock) {
  auto endpoint = sock.remote_endpoint();
  std::string idString = endpoint.address().to_string();
  return std::hash<std::string>{}(idString);
}

Order generateOrder() {
  Order o;
  for (int i = 0; i < 4; ++i) {
    // Generate a random character (A-Z or a-z)
    char random_char = (std::rand() % 2) ? ('A' + std::rand() % 26) : ('a' + std::rand() % 26);
    o.ticker[i] = random_char;
  }
  o.price = std::rand() % 512;
  o.quantity = std::rand() % 64;
  return o;
}

uint64_t timeStampWeak() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::high_resolution_clock::now().time_since_epoch())
      .count();
}

} // namespace hft::utils
