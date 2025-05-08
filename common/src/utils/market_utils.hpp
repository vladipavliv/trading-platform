/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_MARKETUTILS_HPP
#define HFT_COMMON_MARKETUTILS_HPP

#include "constants.hpp"
#include "domain_types.hpp"
#include "rng.hpp"
#include "types.hpp"

namespace hft::utils {

Price fluctuateThePrice(Price price) {
  const int32_t delta = price * PRICE_FLUCTUATION_RATE / 100;
  const int32_t fluctuation = RNG::rng(delta * 2) - delta;
  return price + fluctuation;
}

} // namespace hft::utils

#endif // HFT_COMMON_MARKETUTILS_HPP
