/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_TYPES_HPP
#define HFT_COMMON_TYPES_HPP

#include <cstdint>
#include <expected>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <thread>
#include <vector>

#include <spdlog/common.h>

#include "constants.hpp"
#include "status_code.hpp"

namespace hft {

using String = std::string;
using StringView = std::string_view;
using ConnectionId = uint64_t;
using ObjectId = uint64_t;
using ByteBuffer = std::vector<uint8_t>;
using SPtrByteBuffer = std::shared_ptr<ByteBuffer>;
using ThreadId = uint8_t;
using CoreId = uint8_t;
using Thread = std::jthread;
using Timestamp = uint64_t;
using LogLevel = spdlog::level::level_enum;
using Token = uint64_t;

enum class State : uint8_t { On, Off, Error };

/**
 * Pointer types
 */
template <typename Type>
using SPtr = std::shared_ptr<Type>;

template <typename Type>
using WPtr = std::weak_ptr<Type>;

template <typename Type>
using UPtr = std::unique_ptr<Type>;

template <typename Type>
using CRef = const Type &;

template <typename Type>
using Opt = std::optional<Type>;

/**
 * General template types
 */
template <typename Type>
using Span = std::span<Type>;
using ByteSpan = Span<uint8_t>;

template <typename K, typename V>
using HashMap = std::unordered_map<K, V>;

template <typename Type>
using Atomic = std::atomic<Type>;

template <typename Type>
using Expected = std::expected<Type, StatusCode>;

/**
 * Function types
 */
using Callback = std::function<void()>;
using WeakCallback = std::weak_ptr<Callback>;
using SharedCallback = std::shared_ptr<Callback>;
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
  []<typename... Types>(
      std::tuple<Types...>) -> std::bool_constant<(std::is_same_v<Type, Types> || ...)> {
  }(std::declval<Tuple>());
};

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

#endif // HFT_COMMON_TYPES_HPP