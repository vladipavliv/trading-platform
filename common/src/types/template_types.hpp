/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_TEMPLATETYPES_HPP
#define HFT_COMMON_TEMPLATETYPES_HPP

#include <algorithm>
#include <boost/lockfree/queue.hpp>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "types.hpp"

namespace hft {

/**
 * Span
 */
template <typename Type>
using Span = std::span<Type>;
template <typename Type, typename Cmp>
Span<Type> frontSubspan(Span<Type> input, Cmp cmp = Cmp{}) {
  if (input.size() < 2) {
    return input;
  }
  size_t end = 0;
  while (end < input.size() && cmp(input.front(), input[end])) {
    ++end;
  }
  return input.subspan(0, end);
}

/**
 * handler types
 */
using Callback = std::function<void()>;
template <typename ArgType>
using CRefHandler = std::function<void(const ArgType &)>;
template <typename ArgType>
using SpanHandler = std::function<void(Span<ArgType>)>;

/**
 * lock-free types
 */
template <typename EventType>
using LFQueue = boost::lockfree::queue<EventType>;
template <typename EventType>
using UPtrLFQueue = std::unique_ptr<LFQueue<EventType>>;

template <typename EventType>
static UPtrLFQueue<EventType> createLFQueue(std::size_t size) {
  return std::make_unique<LFQueue<EventType>>(size);
}
template <typename... TupleTypes>
static std::tuple<UPtrLFQueue<TupleTypes>...> createLFQueueTuple(std::size_t size) {
  return std::make_tuple(createLFQueue<TupleTypes>(size)...);
}
template <typename Type, typename... Types>
static constexpr bool contains() {
  return (std::is_same<Type, Types>::value || ...);
}
template <typename... Type>
bool isTupleEmpty(const std::tuple<UPtrLFQueue<Type>...> &tupl) {
  return std::apply([](const auto &...lfqs) { return (... && lfqs->empty()); }, tupl);
}

/**
 * Padding helper
 */
template <typename... ValueTypes>
struct Padding {
  static constexpr size_t DataSize = (sizeof(ValueTypes) + ... + 0);
  static constexpr size_t PaddingSize =
      (CACHE_LINE_SIZE > DataSize) ? (CACHE_LINE_SIZE - DataSize) : 0;

  std::array<char, PaddingSize> padding{};
};

} // namespace hft

#endif // HFT_COMMON_TEMPLATETYPES_HPP