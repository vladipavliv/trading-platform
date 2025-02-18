/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#include "rng.hpp"

namespace hft::utils {

std::random_device RNG::kRandomDevice;
std::mt19937 RNG::kRandomAlgo{kRandomDevice()};

} // namespace hft::utils
