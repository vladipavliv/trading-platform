/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include <cmath>
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

  pthread_t thread = pthread_self();
  int result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
  if (result != 0) {
    spdlog::error("Failed to set thread affinity {}", result);
  }
}

void setTheadRealTime() {
  struct sched_param param;
  param.sched_priority = 99;
  sched_setscheduler(0, SCHED_FIFO, &param);

  auto code = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
  if (code != 0) {
    spdlog::error("Failed to set real-time priority: {}", code);
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

TraderId getTraderId(const TcpSocket &sock) {
  auto endpoint = sock.remote_endpoint();
  std::string idString = endpoint.address().to_string();
  return static_cast<uint32_t>(std::hash<std::string>{}(idString));
}

Order createOrder(TraderId trId, const Ticker &tkr, Quantity quan, Price price, OrderAction act) {
  return {trId, getLinuxTimestamp(), tkr, quan, price, act};
}

Ticker generateTicker() {
  Ticker ticker{};
  for (int i = 0; i < TICKER_SIZE; ++i) {
    ticker[i] = 'A' + RNG::rng(26);
  }
  return ticker;
}

Order generateOrder(Ticker ticker) {
  static size_t traderId = 0;
  Order order;
  order.traderId = traderId++;
  order.action = RNG::rng(1) == 0 ? OrderAction::Buy : OrderAction::Sell;
  order.id = getLinuxTimestamp();
  order.ticker = ticker;
  order.price = RNG::rng(7000);
  order.quantity = RNG::rng(100);
  return order;
}

TickerPrice generatePriceUpdate() {
  TickerPrice price;
  price.ticker = generateTicker();
  price.price = RNG::rng(700);
  return price;
}

uint32_t getLinuxTimestamp() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<uint32_t>(ts.tv_sec * 1'000'000'000 + ts.tv_nsec);
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

UdpSocket createUdpSocket(IoContext &ctx, bool broadcast, Port port) {
  UdpSocket socket(ctx, Udp::v4());
  socket.set_option(boost::asio::socket_base::reuse_address{true});
  if (broadcast) {
    socket.set_option(boost::asio::socket_base::broadcast(true));
  } else {
    socket.bind(UdpEndpoint(Udp::v4(), port));
  }
  return socket;
}

void coreWarmUpJob() {
  /**
   * Running this warm up for 5s does nothing
   * Sometimes server runs stably at ~65k rps for 5us trade rate for a good minute
   * and after a couple of restarts rps might jump to 100k and stay there.
   * Strange stuff. But this warm up is useless.
   */
  long long dummyCounter = 0;
  for (int i = 0; i < 1000000; ++i) {
    dummyCounter += std::sin(i) * std::cos(i);
  }
  spdlog::trace("Warmup job dummy counter {}", dummyCounter);
}

} // namespace hft::utils
