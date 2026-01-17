/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_SHMCONCEPTS_HPP
#define HFT_COMMON_SHMCONCEPTS_HPP

#include <concepts>

namespace hft {

template <typename T>
concept Drainable = requires(T &t) {
  { t.tryDrain() } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept RefCountedShm = requires(T t) {
  { t.increment() } -> std::same_as<void>;
  { t.decrement() } -> std::same_as<bool>;
  { t.count() } -> std::same_as<std::size_t>;
};

} // namespace hft

#endif // HFT_COMMON_SHMCONCEPTS_HPP