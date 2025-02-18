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
    std::uniform_int_distribution<Type> dis(0, number);
    return dis(kRandomAlgo);
  }

  template <typename Type>
  static typename std::enable_if<std::is_floating_point<Type>::value, Type>::type rng(Type number) {
    std::uniform_real_distribution<Type> dis(0.0, number);
    return dis(kRandomAlgo);
  }

private:
  static std::random_device kRandomDevice;
  static std::mt19937 kRandomAlgo;
};

} // namespace hft::utils

#endif // HFT_COMMON_RNG_HPP