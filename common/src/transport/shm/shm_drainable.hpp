/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_SHMDRAINABLE_HPP
#define HFT_COMMON_SHMDRAINABLE_HPP

#include <concepts>

namespace hft {

/**
 * @brief
 */
template <typename T>
concept Drainable = requires(T &t) {
  { t.tryDrain() } -> std::convertible_to<std::size_t>;
};

} // namespace hft

#endif // HFT_COMMON_SHMDRAINABLE_HPP