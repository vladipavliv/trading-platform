/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_RNG_HPP
#define HFT_COMMON_RNG_HPP

#include <cstdint>
#include <random>

namespace hft::utils {

/**
 * @brief Random number generator
 */
class RNG {
public:
  template <typename Type>
  static typename std::enable_if<std::is_integral<Type>::value, Type>::type rng(Type number) {
    thread_local std::mt19937 generator{std::random_device{}()};
    thread_local std::uniform_int_distribution<Type> distribution;

    distribution.param(typename std::uniform_int_distribution<Type>::param_type(0, number));
    return distribution(generator);
  }

  template <typename Type>
  static typename std::enable_if<std::is_floating_point<Type>::value, Type>::type rng(Type number) {
    thread_local std::mt19937 generator{std::random_device{}()};
    thread_local std::uniform_real_distribution<Type> distribution;

    distribution.param(typename std::uniform_real_distribution<Type>::param_type(0.0, number));
    return distribution(generator);
  }
};

} // namespace hft::utils

#endif // HFT_COMMON_RNG_HPP