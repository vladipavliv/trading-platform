/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include <cmath>
#include <ctime>
#include <functional>
#include <iostream>
#include <random>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "logging.hpp"
#include "rng.hpp"
#include "utils.hpp"

namespace hft::utils {

void pinThreadToCore(size_t coreId) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(coreId, &cpuset);

  int result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (result != 0) {
    LOG_ERROR("Failed to pin thread to core: {}, error: {}", coreId, result);
  }
}

void setTheadRealTime(size_t coreId) {
  struct sched_param param;
  param.sched_priority = 99;
  sched_setscheduler(0, SCHED_FIFO, &param);

  auto code = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
  if (code != 0) {
    LOG_ERROR("Failed to set real-time priority on the core: {}, error: {}", coreId, code);
  }
}

void unblockConsole() {
  fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
  std::cout << std::unitbuf;
}

std::string getConsoleInput() {
  std::string cmd;
  struct pollfd fds = {STDIN_FILENO, POLLIN, 0};
  if (poll(&fds, 1, 0) == 1) {
    std::getline(std::cin, cmd);
  }
  return cmd;
}

size_t getTickerHash(const Ticker &ticker) {
  return std::hash<std::string_view>{}(std::string_view(ticker.data(), ticker.size()));
}

Ticker generateTicker() {
  Ticker ticker{};
  for (int i = 0; i < TICKER_SIZE; ++i) {
    ticker[i] = 'A' + RNG::rng(26);
  }
  return ticker;
}

Order generateOrder() {
  const OrderAction action = RNG::rng<uint8_t>(1) == 0 ? OrderAction::Buy : OrderAction::Sell;
  return Order{RNG::rng<uint64_t>(INT_MAX),
               RNG::rng<uint64_t>(INT_MAX),
               RNG::rng<uint64_t>(INT_MAX),
               getTimestamp(),
               generateTicker(),
               RNG::rng<uint32_t>(100),
               RNG::rng<uint32_t>(7000),
               action};
}

TickerPrice generatePriceUpdate() { return {generateTicker(), RNG::rng<uint32_t>(700)}; }

Timestamp getTimestamp() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000 + ts.tv_nsec;
}

void printRawBuffer(const uint8_t *buffer, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i])
              << " ";
  }
  std::cout << std::dec << std::endl;
}

std::string getScaleMs(size_t milliSec) {
  if (milliSec < 1000) {
    return std::to_string(milliSec) + "ms";
  }
  milliSec /= 1000;
  if (milliSec < 60) {
    return std::to_string(milliSec) + "s";
  }
  milliSec /= 60;
  if (milliSec < 60) {
    return std::to_string(milliSec) + "m";
  }
  return "eternity";
}

std::string getScaleUs(size_t microSec) {
  if (microSec < 1000) {
    return std::to_string(microSec) + "us";
  }
  return getScaleMs(microSec / 1000);
}

std::string getScaleNs(size_t nanoSec) {
  if (nanoSec < 1000) {
    return std::to_string(nanoSec) + "ns";
  }
  return getScaleUs(nanoSec / 1000);
}

UdpSocket createUdpSocket(IoCtx &ctx, bool broadcast, Port port) {
  UdpSocket socket(ctx, Udp::v4());
  socket.set_option(boost::asio::socket_base::reuse_address{true});
  if (broadcast) {
    socket.set_option(boost::asio::socket_base::broadcast(true));
  } else {
    socket.bind(UdpEndpoint(Udp::v4(), port));
  }
  return socket;
}

OrderId generateOrderId() {
  static std::atomic_uint64_t counter = 0;
  return counter.fetch_add(1, std::memory_order_relaxed);
}

SocketId generateSocketId() {
  static std::atomic_uint64_t counter = 0;
  return counter.fetch_add(1, std::memory_order_relaxed);
}

Token generateSessionToken() {
  static std::atomic_uint64_t counter = 0;
  return counter.fetch_add(1, std::memory_order_relaxed);
}

} // namespace hft::utils
