/**
 * @author Vladimir Pavliv
 * @date 2025-12-30
 */

#ifndef HFT_TESTS_POSTTRACKER_HPP
#define HFT_TESTS_POSTTRACKER_HPP

#include "primitive_types.hpp"
#include "utils/sync_utils.hpp"

namespace hft::tests {

struct alignas(64) PostTracker {
  explicit PostTracker(size_t target = 0) : target{target}, processed{0}, signal{0} {}

  const size_t target;
  alignas(64) AtomicUInt32 processed;
  alignas(64) AtomicUInt32 signal;
  alignas(64) AtomicBool flag;
  alignas(64) AtomicBool shutdown;

  template <typename Message>
  void post(const Message &) {
    auto counter = processed.fetch_add(1, std::memory_order_relaxed);
    if (counter + 1 >= target) {
      signal.store(1, std::memory_order_release);
      utils::futexWake(signal);
    }
  }

  void wait() { utils::hybridWait(signal, 0, flag, shutdown); }
  void clear() { signal.store(0, std::memory_order_relaxed); }
};

} // namespace hft::tests

#endif // HFT_TESTS_POSTTRACKER_HPP