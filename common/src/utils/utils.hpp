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
#include "domain_types.hpp"
#include "types.hpp"

namespace hft::utils {

void pinThreadToCore(size_t coreId);
void setTheadRealTime();

void unblockConsole();
std::string getConsoleInput();

size_t getTickerHash(const Ticker &ticker);
Ticker generateTicker();
TickerPrice generateTickerPrice();

Timestamp getTimestamp();

std::string getScaleMs(size_t);
std::string getScaleUs(size_t);
std::string getScaleNs(size_t);

void printRawBuffer(const uint8_t *buffer, size_t size);

OrderId generateOrderId();
ConnectionId generateConnectionId();
Token generateToken();
ByteBuffer parse(CRef<String> input);

/**
 * @brief Generic template helpers
 */
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
template <typename Type, typename... Types>
static constexpr bool contains = (std::is_same_v<Type, Types> || ...);

template <typename Type>
struct is_smart_ptr : std::false_type {};

template <typename Type>
struct is_smart_ptr<std::shared_ptr<Type>> : std::true_type {};

template <typename Type>
struct is_smart_ptr<std::unique_ptr<Type>> : std::true_type {};

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

/**
 * @brief Returns subspan of the same elements for the given comparator
 */
template <typename Type, typename Cmp>
std::pair<Span<Type>, Span<Type>> frontSubspan(Span<Type> input, Cmp cmp = Cmp{}) {
  if (input.size() < 2) {
    return std::make_pair(input, Span<Type>());
  }
  const auto base = input.front();
  size_t end = 1;
  while (end < input.size() && cmp(base, input[end]) && cmp(input[end], base)) {
    ++end;
  }
  return std::make_pair(input.subspan(0, end), input.subspan(end, input.size() - end));
}

template <typename Number>
std::string thousandify(Number input) {
  std::stringstream ss;
  ss.imbue(std::locale("en_US.UTF-8"));
  ss << std::fixed << input;
  return ss.str();
}

} // namespace hft::utils

#endif // HFT_COMMON_UTILITIES_HPP
