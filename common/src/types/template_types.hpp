/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_TEMPLATETYPES_HPP
#define HFT_COMMON_TEMPLATETYPES_HPP

#include <algorithm>
#include <array>
#include <boost/lockfree/queue.hpp>
#include <concepts>
#include <functional>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "constants.hpp"
#include "types.hpp"

namespace hft {

/**
 * General template types
 */
template <typename Type>
using Span = std::span<Type>;

template <typename Type>
using CRef = const Type &;

template <typename K, typename V>
using HashMap = std::unordered_map<K, V>;

template <typename Type>
using Atomic = std::atomic<Type>;

/**
 * Function types
 */
using Callback = std::function<void()>;
using Predicate = std::function<bool()>;
template <typename ArgType>
using Handler = std::function<void(ArgType)>;
template <typename ArgType>
using CRefHandler = std::function<void(const ArgType &)>;
template <typename ArgType>
using SpanHandler = std::function<void(Span<ArgType>)>;

/**
 * Concepts
 */
template <typename EventType>
concept UnorderedMapKey = requires(EventType event) {
  { std::hash<EventType>{}(event) } -> std::convertible_to<std::size_t>;
} && std::equality_comparable<EventType>;

template <typename Type, typename Tuple>
concept IsTypeInTuple = requires {
  []<typename... Types>(std::tuple<Types...>) {
    return (std::is_same_v<Type, Types> || ...);
  }(std::declval<Tuple>());
};

/**
 * Lock free queue types
 */
template <typename EventType>
using LFQueue = boost::lockfree::queue<EventType>;
template <typename EventType>
using UPtrLFQueue = std::unique_ptr<LFQueue<EventType>>;
template <typename EventType>
using SPtrLFQueue = std::shared_ptr<LFQueue<EventType>>;

/**
 * Padding type
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