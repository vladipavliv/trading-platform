/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_TEMPLATEUTILS_HPP
#define HFT_COMMON_TEMPLATEUTILS_HPP

#include <algorithm>
#include <boost/lockfree/queue.hpp>
#include <functional>
#include <memory>
#include <span>
#include <sstream>
#include <string>
#include <vector>

#include "constants.hpp"
#include "template_types.hpp"
#include "types.hpp"

namespace hft::utils {

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

/**
 * @brief lock-free helpers
 */
template <typename EventType>
static UPtrLFQueue<EventType> createLFQueue(std::size_t size) {
  return std::make_unique<LFQueue<EventType>>(size);
}
template <typename... TupleTypes>
static std::tuple<UPtrLFQueue<TupleTypes>...> createLFQueueTuple(std::size_t size) {
  return std::make_tuple(createLFQueue<TupleTypes>(size)...);
}

template <typename MessageType, typename = void>
struct HasTraderId : std::false_type {};

// Specialization if T has member 'traderId'
template <typename MessageType>
struct HasTraderId<MessageType, std::void_t<decltype(std::declval<MessageType>().traderId)>>
    : std::true_type {};

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

template <typename Number>
std::string thousandify(Number input) {
  std::stringstream ss;
  ss.imbue(std::locale("en_US.UTF-8"));
  ss << std::fixed << input;
  return ss.str();
}

} // namespace hft::utils

#endif // HFT_COMMON_TEMPLATEUTILS_HPP