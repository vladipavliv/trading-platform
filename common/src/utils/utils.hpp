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

template <typename Type>
inline uintptr_t getId(const Type *obj) {
  return reinterpret_cast<uintptr_t>(obj);
}

void pinThreadToCore(int core_id);
void setTheadRealTime();

size_t getTraderId(const TcpSocket &sock);

inline size_t generateOrderId() {
  static std::atomic<size_t> counter{0};
  return counter.fetch_add(1, std::memory_order_relaxed);
};

uint32_t generateNumber(uint32_t val);
Ticker generateTicker();
Order generateOrder();
PriceUpdate generatePriceUpdate();

uint64_t timeStampWeak();

uint64_t getLinuxTimestamp();

} // namespace hft::utils

#endif // HFT_COMMON_UTILITIES_HPP
