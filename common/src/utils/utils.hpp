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
#include <linux/futex.h>
#include <pthread.h>
#include <sched.h>
#include <sys/syscall.h>
#include <thread>
#include <typeinfo>
#include <unistd.h>
#include <x86intrin.h>

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

inline auto getCycles() -> Timestamp {
  unsigned int dummy;
  return __builtin_ia32_rdtscp(&dummy);
}

inline void pinThreadToCore(size_t coreId) {
  if (coreId == 0) {
    throw std::runtime_error("Cannot pin thread to core 0");
  }
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
  static std::atomic<OrderId> id{0};
  return id.fetch_add(1);
}

inline auto generateConnectionId() -> ConnectionId {
  static std::atomic<ConnectionId> id{0};
  return id.fetch_add(1);
}

inline auto generateToken() -> Token {
  static std::atomic<Token> counter{0};
  return getTimestamp() + counter.fetch_add(1);
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

inline void futexWait(std::atomic<uint32_t> &futex) {
  const uint32_t val = futex.load(std::memory_order_acquire);
  syscall(SYS_futex, reinterpret_cast<uint32_t *>(&futex), FUTEX_WAIT_PRIVATE, val, nullptr,
          nullptr, 0);
}

inline void futexWake(std::atomic<uint32_t> &futex) {
  syscall(SYS_futex, reinterpret_cast<uint32_t *>(&futex), FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr,
          0);
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
  }
}

template <typename EventType, typename... Types>
constexpr size_t getTypeIndex() {
  return indexOf<EventType, Types...>();
}

template <typename DurationType>
inline Seconds toSeconds(DurationType duration) {
  return std::chrono::duration_cast<std::chrono::seconds>(duration);
}

inline Price fluctuateThePrice(Price price) {
  const int32_t delta = price * PRICE_FLUCTUATION_RATE / 100;
  const int32_t fluctuation = RNG::generate(0, delta * 2) - delta;
  return price + fluctuation;
}

struct alignas(64) Consumer {
  Consumer(size_t target = 0) : target{target} {}

  const size_t target;
  alignas(64) std::atomic<uint64_t> processed;
  alignas(64) std::atomic_flag signal;

  template <typename Message>
  void post(const Message &msg) {
    auto counter = processed.fetch_add(1);
    if (counter == target) {
      signal.test_and_set(std::memory_order_release);
    }
  }
};

struct OpCounter {
  std::atomic<size_t> *value{nullptr};

  explicit OpCounter(std::atomic<size_t> *v) : value{v} {
    if (value) {
      value->fetch_add(1);
    }
  }

  OpCounter(OpCounter &&other) noexcept : value(other.value) { other.value = nullptr; }

  OpCounter &operator=(OpCounter &&other) = delete;

  ~OpCounter() {
    if (value)
      value->fetch_sub(1);
  }

  OpCounter(const OpCounter &) = delete;
  OpCounter &operator=(const OpCounter &) = delete;
};

} // namespace hft::utils

#endif // HFT_COMMON_UTILITIES_HPP
