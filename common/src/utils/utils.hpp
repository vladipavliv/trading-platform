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

#include "boost_types.hpp"
#include "market_types.hpp"
#include "network_types.hpp"
#include "types.hpp"

namespace hft::utils {

void pinThreadToCore(size_t coreId);
void setTheadRealTime(size_t coreId);

void unblockConsole();
std::string getConsoleInput();

size_t getTickerHash(const Ticker &ticker);
Ticker generateTicker();
Order generateOrder();
TickerPrice generateTickerPrice();

Timestamp getTimestamp();

std::string getScaleMs(size_t);
std::string getScaleUs(size_t);
std::string getScaleNs(size_t);

void printRawBuffer(const uint8_t *buffer, size_t size);

UdpSocket createUdpSocket(IoCtx &ctx, bool broadcast = true, Port port = 0);

OrderId generateOrderId();
SocketId generateSocketId();
SocketId generateSessionToken();

} // namespace hft::utils

#endif // HFT_COMMON_UTILITIES_HPP
