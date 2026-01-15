/**
 * @author Vladimir Pavliv
 * @date 2026-01-07
 */

#ifndef HFT_COMMON_TESTUTILS_HPP
#define HFT_COMMON_TESTUTILS_HPP

#include <atomic>

#include "container_types.hpp"
#include "domain_types.hpp"
#include "id_utils.hpp"
#include "rng.hpp"
#include "sync_utils.hpp"
#include "time_utils.hpp"

namespace hft::utils {

inline Price fluctuateThePrice(Price price) {
  const int32_t delta = price * PRICE_FLUCTUATION_RATE / 100;
  const int32_t fluctuation = RNG::generate(0, delta * 2) - delta;
  return price + fluctuation;
}

} // namespace hft::utils

#endif // HFT_COMMON_TESTUTILS_HPP
