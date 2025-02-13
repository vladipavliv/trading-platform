/**
 * @file padded_counter.hpp
 * @brief Counter to avoid false sharing
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_COUNTER_HPP
#define HFT_COMMON_COUNTER_HPP

#include <atomic>
#include <new>

namespace hft {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winterference-size"
struct PaddedCounter {
  alignas(std::hardware_destructive_interference_size) std::atomic_size_t value{0};
  char pad[std::hardware_destructive_interference_size - sizeof(std::atomic_size_t)];
};
#pragma GCC diagnostic pop

} // namespace hft

#endif // HFT_SERVER_