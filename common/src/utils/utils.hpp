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

void pinThreadToCore(int core_id);
void setTheadRealTime();

void unblockConsole();
std::string getConsoleInput();

TraderId getTraderId(const TcpSocket &sock);

size_t getTickerHash(const Ticker &ticker);
Order createOrder(TraderId trId, const Ticker &tkr, Quantity quan, Price price, OrderAction act);
Ticker generateTicker();
Order generateOrder(Ticker ticker);
TickerPrice generateTickerPrice();

uint32_t getLinuxTimestamp();
std::string getScaleMs(size_t);
std::string getScaleUs(size_t);
std::string getScaleNs(size_t);

void printRawBuffer(const uint8_t *buffer, size_t size);

UdpSocket createUdpSocket(IoContext &ctx, bool broadcast = true, Port port = 0);

void coreWarmUpJob();

} // namespace hft::utils

#endif // HFT_COMMON_UTILITIES_HPP
