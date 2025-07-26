/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_RNG_HPP
#define HFT_COMMON_RNG_HPP

#include <concepts>
#include <cstdint>
#include <random>

namespace hft::utils {

/**
 * @brief Random number generator
 */
class RNG {
public:
  template <std::integral Type>
  static Type generate(Type min, Type max) {
    thread_local std::mt19937 generator{std::random_device{}()};

    std::uniform_int_distribution<Type> distribution(min, max);
    return distribution(generator);
  }

  template <std::floating_point Type>
  static Type generate(Type min, Type max) {
    thread_local std::mt19937 generator{std::random_device{}()};

    std::uniform_real_distribution<Type> distribution(min, max);
    return distribution(generator);
  }
};

} // namespace hft::utils

#endif // HFT_COMMON_RNG_HPP