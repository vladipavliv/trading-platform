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

Order createOrder(TraderId trId, const Ticker &tkr, Quantity quan, Price price, OrderAction act);
Ticker generateTicker();
Order generateOrder(Ticker ticker);
TickerPrice generateTickerPrice();

uint64_t timeStampWeak();

// Use only half for efficieny, precise enough for our needs
uint32_t getLinuxTimestamp();
std::string getScaleMs(size_t);
std::string getScaleUs(size_t);
std::string getScaleNs(size_t);

size_t getId();
void printRawPuffer(const uint8_t *buffer, size_t size);

} // namespace hft::utils

#endif // HFT_COMMON_UTILITIES_HPP
