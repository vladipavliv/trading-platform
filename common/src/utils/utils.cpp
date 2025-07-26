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

void setTheadRealTime() {
  struct sched_param param;
  param.sched_priority = 99;
  sched_setscheduler(0, SCHED_FIFO, &param);

  auto code = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
  if (code != 0) {
    LOG_ERROR("Failed to set real-time priority on the error: {}", code);
  }
}

auto generateOrderId() -> OrderId {
  static std::atomic_uint64_t counter = 0;
  return counter.fetch_add(1, std::memory_order_relaxed);
}

auto generateConnectionId() -> ConnectionId {
  static std::atomic_uint64_t counter = 0;
  return counter.fetch_add(1, std::memory_order_relaxed);
}

auto generateToken() -> Token {
  static std::atomic_uint64_t counter = 0;
  return getTimestamp() + counter.fetch_add(1, std::memory_order_relaxed);
}

auto getTimestamp() -> Timestamp {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<uint64_t>(ts.tv_sec) * 1'000'000 + ts.tv_nsec / 1'000;
}

auto createUdpSocket(IoCtx &ctx, bool broadcast, Port port) -> UdpSocket {
  UdpSocket socket(ctx, Udp::v4());
  socket.set_option(boost::asio::socket_base::reuse_address{true});
  if (broadcast) {
    socket.set_option(boost::asio::socket_base::broadcast(true));
  } else {
    socket.bind(UdpEndpoint(Udp::v4(), port));
  }
  return socket;
}

auto split(CRef<String> input) -> ByteBuffer {
  ByteBuffer result;
  std::stringstream ss(input);
  std::string token;

  while (std::getline(ss, token, ',')) {
    result.push_back(static_cast<uint8_t>(std::stoi(token)));
  }
  return result;
}

} // namespace hft::utils
