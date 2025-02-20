/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_UTILITIES_HPP
#define HFT_COMMON_UTILITIES_HPP

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cxxabi.h>
#include <pthread.h>
#include <sched.h>
#include <thread>
#include <typeinfo>

#include "market_types.hpp"
#include "network_types.hpp"
#include "types.hpp"

namespace hft::utils {

void pinThreadToCore(int core_id);
void setTheadRealTime();

TraderId getTraderId(const TcpSocket &sock);

inline size_t generateOrderId() {
  static std::atomic<size_t> counter{0};
  return counter.fetch_add(1, std::memory_order_relaxed);
};

Order createOrder(TraderId trId, Ticker tkr, OrderAction act, Quantity quan, Price price);
Ticker generateTicker();
Order generateOrder(Ticker ticker);
TickerPrice generateTickerPrice();

uint64_t timeStampWeak();

uint64_t getLinuxTimestamp();
std::string getScaleMs(size_t);
std::string getScaleUs(size_t);
std::string getScaleNs(size_t);

size_t getId();
void printRawPuffer(const uint8_t *buffer, size_t size);

template <typename EventType, typename First, typename... Rest>
constexpr size_t indexOf() {
  if constexpr (std::is_same_v<EventType, First>) {
    return 0;
  } else if constexpr (sizeof...(Rest) > 0) {
    return 1 + indexOf<EventType, Rest...>();
  } else {
    static_assert(sizeof...(Rest) > 0, "EventType not found in EventTypes pack.");
    return -1;
  }
}

template <typename EventType, typename... Types>
constexpr size_t getTypeIndex() {
  return indexOf<EventType, Types...>();
}

} // namespace hft::utils

#endif // HFT_COMMON_UTILITIES_HPP
