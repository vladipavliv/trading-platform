/**
 * @author Vladimir Pavliv
 * @date 2025-03-21
 */

#ifndef HFT_COMMON_BUSABLE_HPP
#define HFT_COMMON_BUSABLE_HPP

#include <concepts>
#include <utility>

#include "domain_types.hpp"

namespace hft {

/**
 * @brief Concept for a Bus that accepts any types to be posted
 * Checks if ::post works for local type, which would mean it works for any type
 */
template <typename Bus>
concept Busable = [] {
  struct ProbeType {};
  return requires(Bus &bus) {
    { bus.template post<ProbeType>(std::declval<const ProbeType &>()) };
  };
}();

template <typename Bus, typename... Types>
concept BusableFor =
    (requires(Bus &bus) { bus.template post<Types>(std::declval<Types>()); } && ...);

template <typename Bus, typename... Types>
concept BusableForAny =
    (requires(Bus &bus) { bus.template post<Types>(std::declval<Types>()); } || ...);

} // namespace hft

#endif // HFT_COMMON_BUSABLE_HPP
