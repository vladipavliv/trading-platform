/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_UTILITIES_HPP
#define HFT_COMMON_UTILITIES_HPP

#include <atomic>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <cxxabi.h>
#include <pthread.h>
#include <sched.h>
#include <thread>
#include <typeinfo>

#include "boost_types.hpp"
#include "domain_types.hpp"
#include "logging.hpp"
#include "rng.hpp"
#include "types.hpp"

namespace hft::utils {

inline auto getTimestamp() -> Timestamp {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<uint64_t>(ts.tv_sec) * 1'000'000 + ts.tv_nsec / 1'000;
}

inline auto getTimestampNs() -> Timestamp {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000 + ts.tv_nsec;
}

inline void pinThreadToCore(size_t coreId) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(coreId, &cpuset);

  int result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (result != 0) {
    LOG_ERROR("Failed to pin thread to core: {}, error: {}", coreId, result);
  }
}

inline void setTheadRealTime() {
  struct sched_param param;
  param.sched_priority = 99;
  sched_setscheduler(0, SCHED_FIFO, &param);

  auto code = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
  if (code != 0) {
    LOG_ERROR("Failed to set real-time priority on the error: {}", code);
  }
}

inline auto generateOrderId() -> OrderId {
  static std::atomic_uint64_t counter = 0;
  return counter.fetch_add(1, std::memory_order_relaxed);
}

inline auto generateConnectionId() -> ConnectionId {
  static std::atomic_uint64_t counter = 0;
  return counter.fetch_add(1, std::memory_order_relaxed);
}

inline auto generateToken() -> Token {
  static std::atomic_uint64_t counter = 0;
  return getTimestamp() + counter.fetch_add(1, std::memory_order_relaxed);
}

inline auto createUdpSocket(IoCtx &ctx, bool broadcast = true, Port port = 0) -> UdpSocket {
  UdpSocket socket(ctx, Udp::v4());
  socket.set_option(boost::asio::socket_base::reuse_address{true});
  if (broadcast) {
    socket.set_option(boost::asio::socket_base::broadcast(true));
  } else {
    socket.bind(UdpEndpoint(Udp::v4(), port));
  }
  return socket;
}

inline auto split(CRef<String> input) -> ByteBuffer {
  ByteBuffer result;
  std::stringstream ss(input);
  std::string token;

  while (std::getline(ss, token, ',')) {
    result.push_back(static_cast<uint8_t>(std::stoi(token)));
  }
  return result;
}

template <typename Type, typename... Types>
static constexpr bool contains = (std::is_same_v<Type, Types> || ...);

template <size_t First>
constexpr bool is_ascending() {
  return true;
}

template <size_t First, size_t Second, size_t... Rest>
constexpr bool is_ascending() {
  if constexpr (sizeof...(Rest) == 0) {
    return First <= Second;
  } else {
    return (First <= Second) && is_ascending<Second, Rest...>();
  }
}

template <typename ValueType>
bool hasIntersection(const std::vector<ValueType> &left, const std::vector<ValueType> &right) {
  return std::any_of(left.begin(), left.end(), [&](const ValueType &value) {
    return std::find(right.begin(), right.end(), value) != right.end();
  });
}

template <typename T>
concept Arithmetic = std::integral<T> || std::floating_point<T>;

/**
 * @brief Funny function doing funny things
 * @returns thousandified number. A number, that has gone through some
 * complex thousandification procedures
 * @todo Could make a custom formatter for spdlog, but that equals time
 */
template <Arithmetic Number>
auto thousandify(Number input) -> String {
  std::stringstream ss;
  ss.imbue(std::locale("en_US.UTF-8"));
  ss << std::fixed << input;
  return ss.str();
}

inline Ticker generateTicker() {
  Ticker result;
  for (size_t i = 0; i < 4; ++i) {
    result[i] = RNG::generate<uint8_t>((uint8_t)'A', (uint8_t)'Z');
  }
  return result;
}

inline Order generateOrder(Ticker ticker = {'G', 'O', 'O', 'G'}) {
  return Order{generateOrderId(),
               getTimestamp(),
               ticker,
               RNG::generate<Quantity>(0, 1000),
               RNG::generate<Price>(10, 10000),
               RNG::generate<uint8_t>(0, 1) == 0 ? OrderAction::Buy : OrderAction::Sell};
}

inline String getEnvVar(CRef<String> varName) {
  const char *value = std::getenv(varName.c_str());
  return value == nullptr ? "" : value;
}

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
