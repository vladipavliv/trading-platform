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
  try {
    auto endpoint = sock.remote_endpoint();
    std::string idString = endpoint.address().to_string();
    return std::hash<std::string>{}(idString) % sizeof(uint16_t);
  } catch (const std::exception &e) {
    spdlog::error("Failed to retrieve socket id: {}", e.what());
    throw;
  }
}

bool socketOk(const TcpSocket &sock) {
  if (!sock.is_open()) {
    spdlog::error("Socket is not opened");
    return false;
  }
  boost::system::error_code ec;
  sock.remote_endpoint(ec);
  if (ec) {
    spdlog::error("Socket is not connected: {}", ec.message());
    return false;
  } else {
    spdlog::info("Socket is ok");
  }
  return true;
}

} // namespace hft::utils