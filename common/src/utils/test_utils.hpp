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

struct alignas(64) Consumer {
  explicit Consumer(size_t target = 0) : target{target}, processed{0}, signal{0} {}

  const size_t target;
  alignas(64) std::atomic<uint64_t> processed;
  alignas(64) std::atomic<uint32_t> signal;
  alignas(64) std::atomic<bool> flag;
  alignas(64) std::atomic<bool> shutdown;

  template <typename Message>
  void post(const Message &) {
    auto counter = processed.fetch_add(1, std::memory_order_relaxed);
    if (counter + 1 >= target) {
      signal.store(1, std::memory_order_release);
      futexWake(signal);
    }
  }

  void wait() { hybridWait(signal, 0, flag, shutdown); }
  void clear() { signal.store(0, std::memory_order_relaxed); }
};

} // namespace hft::utils

#endif // HFT_COMMON_TESTUTILS_HPP
