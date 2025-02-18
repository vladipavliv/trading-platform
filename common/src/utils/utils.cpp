/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include <ctime>
#include <functional>
#include <iostream>
#include <random>
#include <spdlog/spdlog.h>

#include "rng.hpp"
#include "utils.hpp"

namespace hft::utils {

void pinThreadToCore(int core_id) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core_id, &cpuset);

  sched_setaffinity(core_id, sizeof(cpu_set_t), &cpuset);

  pthread_t thread = pthread_self();
  int result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
  if (result != 0) {
    spdlog::error("Failed to set thread affinity");
  }
}

void setTheadRealTime() {
  // Set real-time priority (SCHED_FIFO)
  struct sched_param param;
  param.sched_priority = 99; // Highest priority for real-time threads
  sched_setscheduler(0, SCHED_FIFO, &param);

  auto rc = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
  if (rc != 0) {
    spdlog::error("Failed to set real-time priority: {}", rc);
  }
}

size_t getTraderId(const TcpSocket &sock) {
  auto endpoint = sock.remote_endpoint();
  std::string idString = endpoint.address().to_string();
  return std::hash<std::string>{}(idString);
}

size_t getId() { // For the sake of testing
  static size_t counter = 0;
  return counter++;
}

Order createOrder(TraderId trId, const Ticker &tkr, OrderAction act, Quantity quan, Price price) {
  return {trId, getLinuxTimestamp(), tkr, act, quan, price};
}

Ticker generateTicker() {
  Ticker ticker{};
  for (int i = 0; i < TICKER_SIZE; ++i) {
    ticker[i] = 'A' + RNG::rng(26);
  }
  return ticker;
}

Order generateOrder() {
  Order order;
  order.id = getLinuxTimestamp();
  order.ticker = generateTicker();
  order.price = RNG::rng(700);
  order.quantity = RNG::rng(50);
  return order;
}

TickerPrice generatePriceUpdate() {
  TickerPrice price;
  price.ticker = generateTicker();
  price.price = RNG::rng(700);
  return price;
}

uint64_t timeStampWeak() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::high_resolution_clock::now().time_since_epoch())
      .count();
}

TimestampRaw getLinuxTimestamp() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1'000'000'000 + ts.tv_nsec;
}

std::string getScale(size_t nanoseconds) {
  if (nanoseconds < 1000) {
    return std::to_string(nanoseconds) + "ns";
  }
  nanoseconds /= 1000;
  if (nanoseconds < 1000) {
    return std::to_string(nanoseconds) + "µs";
  }
  nanoseconds /= 1000;
  if (nanoseconds < 1000) {
    return std::to_string(nanoseconds) + "ms";
  }
  nanoseconds /= 1000;
  if (nanoseconds < 60) {
    return std::to_string(nanoseconds) + "s";
  }
  return "eternity";
}

} // namespace hft::utils
