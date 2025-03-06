/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_MARKETUTILS_HPP
#define HFT_COMMON_MARKETUTILS_HPP

#include "constants.hpp"
#include "market_types.hpp"
#include "rng.hpp"
#include "types.hpp"

namespace hft::utils {

Price fluctuateThePrice(Price price) {
  Price delta = price * PRICE_FLUCTUATION_RATE / 100;
  // generate rng in [-delta, delta]
  auto fluctuation = RNG::rng(delta * 2) - delta;
  return price + fluctuation;
}

} // namespace hft::utils

#endif // HFT_COMMON_MARKETUTILS_HPP
