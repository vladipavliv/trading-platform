/**
 * @author Vladimir Pavliv
 * @date 2025-03-21
 */

#ifndef HFT_COMMON_BUSABLE_HPP
#define HFT_COMMON_BUSABLE_HPP

#include <concepts>
#include <utility>

namespace hft {

/**
 * @brief Defines a concept for a bus with templated ::post
 */
template <typename Bus>
concept Busable = [] {
  struct ProbeType {};
  return requires(Bus &bus) {
    { bus.template post<ProbeType>(std::declval<const ProbeType &>()) };
  };
}();

} // namespace hft

#endif // HFT_COMMON_BUSABLE_HPP
