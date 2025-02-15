/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include <ctime>
#include <functional>
#include <iostream>
#include <random>
#include <spdlog/spdlog.h>

#include "utils.hpp"

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

uint8_t generateNumber() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, 255);
  return dis(gen);
}

Ticker generateTicker() {
  Ticker ticker;
  for (int i = 0; i < 4; ++i) {
    ticker[i] = 'A' + generateNumber() % 26;
  }
  return ticker;
}

Order generateOrder() {
  Order order;
  order.ticker = generateTicker();
  order.price = generateNumber();
  order.quantity = generateNumber();
  return order;
}

PriceUpdate generatePriceUpdate() {
  PriceUpdate price;
  price.ticker = generateTicker();
  price.price = generateNumber();
  return price;
}

uint64_t timeStampWeak() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::high_resolution_clock::now().time_since_epoch())
      .count();
}

} // namespace hft::utils
