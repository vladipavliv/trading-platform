/**
 * @author Vladimir Pavliv
 * @date 2026-01-12
 */

#ifndef HFT_COMMON_SHMQUEUE_HPP
#define HFT_COMMON_SHMQUEUE_HPP

#include "containers/sequenced_spsc.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"
#include "ptr_types.hpp"
#include "utils/memory_utils.hpp"
#include "utils/sync_utils.hpp"

namespace hft {

/**
 * @brief lock-free queue + control block for a one-side message stream via shm
 */
struct alignas(utils::HUGE_PAGE_SIZE) ShmQueue {
  // 8mb + control block, place at the start so data fills up 4 full huge pages
  ALIGN_CL SequencedSPSC<128 * 1024> queue;

  ALIGN_CL AtomicUInt32 futex{0};      // futex
  ALIGN_CL uint32_t futexCounter{0};   // optimization to avoid fetch_add
  ALIGN_CL AtomicBool waitFlag{false}; // optimization to hit futex only when necessary
  ALIGN_CL AtomicUInt32 refCount{0};   // counter for shm cleanup

  void notify() {
    if (waitFlag.load(std::memory_order_acquire)) {
      LOG_DEBUG("ShmQueue notify");
      futex.store(++futexCounter, std::memory_order_release);
      utils::futexWake(futex);
    }
  }

  void wait() {
    const auto ftxVal = futex.load(std::memory_order_acquire);
    waitFlag.store(true, std::memory_order_release);
    utils::futexWait(futex, ftxVal);
    waitFlag.store(false, std::memory_order_release);
  }

  void increment() { refCount.fetch_add(1, std::memory_order_release); }

  bool decrement() noexcept {
    uint32_t old = refCount.load(std::memory_order_acquire);
    while (old > 0) {
      if (refCount.compare_exchange_weak( // format
              old, old - 1, std::memory_order_acq_rel, std::memory_order_acquire)) {
        return old == 1;
      }
    }
    LOG_ERROR("Attempted to decrement refCount already at 0");
    return false;
  }

  size_t count() const { return refCount.load(std::memory_order_acquire); }
};

} // namespace hft

#endif // HFT_COMMON_SHMQUEUE_HPP