/**
 * @author Vladimir Pavliv
 * @date 2026-01-07
 */

#ifndef HFT_COMMON_TRAITUTILS_HPP
#define HFT_COMMON_TRAITUTILS_HPP

#include <concepts>
#include <functional>
#include <tuple>
#include <type_traits>

namespace hft::utils {

/**
 * @brief Checks if a type exists within a variadic pack.
 */
template <typename Type, typename... Types>
static constexpr bool contains = (std::is_same_v<Type, Types> || ...);

/**
 * @brief Compile-time check for ascending order of constants.
 */
template <size_t First, size_t... Rest>
constexpr bool is_ascending() {
  if constexpr (sizeof...(Rest) == 0) {
    return true;
  } else {
    auto check = [](size_t f, auto... r) {
      size_t values[] = {r...};
      return f <= values[0];
    };
    return check(First, Rest...) && is_ascending<Rest...>();
  }
}

/**
 * @brief Finds the 0-based index of a type within a variadic pack.
 */
template <typename T, typename First, typename... Rest>
constexpr size_t indexOf() {
  if constexpr (std::is_same_v<T, First>) {
    return 0;
  } else {
    static_assert(sizeof...(Rest) > 0, "Type not found in pack.");
    return 1 + indexOf<T, Rest...>();
  }
}

template <typename EventType, typename... Types>
constexpr size_t getTypeIndex() {
  return indexOf<EventType, Types...>();
}

template <typename EventType>
concept UnorderedMapKey = requires(EventType &event) {
  { std::hash<EventType>{}(event) } -> std::convertible_to<std::size_t>;
} && std::equality_comparable<EventType>;

template <typename Type, typename Tuple>
concept IsTypeInTuple = requires {
  []<typename... Types>(
      std::tuple<Types...>) -> std::bool_constant<(std::is_same_v<Type, Types> || ...)> {
  }(std::declval<Tuple>());
};

} // namespace hft::utils

#endif // HFT_COMMON_TRAITUTILS_HPP
