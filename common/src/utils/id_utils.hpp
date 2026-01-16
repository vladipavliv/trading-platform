/**
 * @author Vladimir Pavliv
 * @date 2026-01-07
 */

#ifndef HFT_COMMON_IDUTILS_HPP
#define HFT_COMMON_IDUTILS_HPP

#include "id/slot_id.hpp"
#include "primitive_types.hpp"

namespace hft::utils {

inline auto genConnectionId() -> uint32_t {
  static uint64_t counter{0};
  return counter++;
}

inline auto genToken() -> uint32_t {
  static uint64_t counter{0};
  return counter++;
}

} // namespace hft::utils

#endif // HFT_COMMON_IDUTILS_HPP
