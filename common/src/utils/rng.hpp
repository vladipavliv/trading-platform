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
  struct FastGen {
    uint64_t state;

    explicit FastGen(uint64_t seed) : state(seed) {}

    uint64_t operator()() {
      uint64_t z = (state += 0x9e3779b97f4a7c15);
      z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
      z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
      return z ^ (z >> 31);
    }
  };

  template <std::integral Type>
  static Type generate(Type min, Type max) {
    thread_local FastGen generator{42}; // Seed once
    if (min > max)
      std::swap(min, max);

    uint64_t range = static_cast<uint64_t>(max - min + 1);
    // Fast bounded integer without modulo bias (Lemire's trick)
    uint64_t x = generator();
    __uint128_t m = static_cast<__uint128_t>(x) * static_cast<__uint128_t>(range);
    return min + static_cast<Type>(m >> 64);
  }

  template <std::floating_point Type>
  static Type generate(Type min, Type max) {
    thread_local FastGen generator{42};
    if (min > max)
      std::swap(min, max);

    // 1. Generate 64 bits of randomness
    uint64_t x = generator();

    // 2. Map to [0.0, 1.0) using bit-casting
    // We take 52 bits of randomness to fill the mantissa of a double
    // This is significantly faster than std::uniform_real_distribution
    double zero_to_one;
    uint64_t tmp = (x >> 12) | 0x3FF0000000000000ULL;
    std::memcpy(&zero_to_one, &tmp, sizeof(double));
    zero_to_one -= 1.0;

    // 3. Scale to [min, max]
    return min + static_cast<Type>(zero_to_one * (max - min));
  }
};

class SlowRNG {
public:
  template <std::integral Type>
  static Type generate(Type min, Type max) {
    thread_local std::mt19937 generator{std::random_device{}()};
    if (min > max) {
      std::swap(min, max);
    }
    std::uniform_int_distribution<Type> distribution(min, max);
    return distribution(generator);
  }

  template <std::floating_point Type>
  static Type generate(Type min, Type max) {
    thread_local std::mt19937 generator{std::random_device{}()};
    if (min > max) {
      std::swap(min, max);
    }
    std::uniform_real_distribution<Type> distribution(min, max);
    return distribution(generator);
  }
};

} // namespace hft::utils

#endif // HFT_COMMON_RNG_HPP