/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_RNG_HPP
#define HFT_COMMON_RNG_HPP

#include <cstdint>
#include <random>

namespace hft::utils {

class RNG {
public:
  template <typename Type>
  static typename std::enable_if<std::is_integral<Type>::value, Type>::type rng(Type number) {
    static std::uniform_int_distribution<Type> distribution(0, 0);
    distribution.param(typename std::uniform_int_distribution<Type>::param_type(0, number));
    return distribution(kRandomAlgo);
  }

  template <typename Type>
  static typename std::enable_if<std::is_floating_point<Type>::value, Type>::type rng(Type number) {
    static std::uniform_real_distribution<Type> distribution(0.0, 0.0);
    distribution.param(typename std::uniform_int_distribution<Type>::param_type(0.0, number));
    return distribution(kRandomAlgo);
  }

private:
  static std::random_device kRandomDevice;
  static std::mt19937 kRandomAlgo;
};

} // namespace hft::utils

#endif // HFT_COMMON_RNG_HPP