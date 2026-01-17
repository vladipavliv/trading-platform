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

struct alignas(utils::HUGE_PAGE_SIZE) ShmQueue {
  // 8mb + control block, place at the start so data fills up 4 full huge pages
  ALIGN_CL SequencedSPSC<128 * 1024> queue;

  ALIGN_CL AtomicUInt32 futex{0};
  ALIGN_CL uint32_t futexCounter{0};
  ALIGN_CL AtomicBool waitFlag{false};
  ALIGN_CL AtomicUInt32 refCount{0};

  void notify() {
    if (waitFlag.load(std::memory_order_acquire)) {
      LOG_DEBUG("ShmQueue notify");
      futex.store(++futexCounter, std::memory_order_release);
      utils::futexWake(futex);
    }
  }

  void wait() {
    LOG_DEBUG("ShmQueue wait");
    const auto ftxVal = futex.load(std::memory_order_acquire);
    waitFlag.store(true, std::memory_order_release);
    utils::futexWait(futex, ftxVal);
    waitFlag.store(false, std::memory_order_release);
  }

  void increment() { refCount.fetch_add(1, std::memory_order_release); }

  bool decrement() noexcept {
    uint32_t old = refCount.load(std::memory_order_acquire);
    while (true) {
      if (old == 0) {
        LOG_ERROR("Attempted to decrement refCount below 0");
        return false;
      }
      if (refCount.compare_exchange_weak( // format
              old, old - 1, std::memory_order_acq_rel, std::memory_order_acquire)) {
        return old == 1;
      }
    }
  }

  size_t count() const { return refCount.load(std::memory_order_acquire); }
};

} // namespace hft

#endif // HFT_COMMON_SHMQUEUE_HPP