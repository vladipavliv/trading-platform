/**
 * @author Vladimir Pavliv
 * @date 2026-01-12
 */

#ifndef HFT_COMMON_SHMQUEUE_HPP
#define HFT_COMMON_SHMQUEUE_HPP

#include "containers/sequenced_spsc.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"
#include "utils/sync_utils.hpp"

namespace hft {

struct ShmQueue {
  alignas(64) AtomicUInt32 futex{0};
  alignas(64) uint64_t futexCounter{0};
  alignas(64) AtomicBool flag{false};
  alignas(64) SequencedSPSC<128 * 1024> queue;

  void notify() {
    if (flag.load(std::memory_order_acquire)) {
      LOG_DEBUG("ShmQueue notify");
      futex.store(++futexCounter, std::memory_order_release);
      utils::futexWake(futex);
    }
  }

  void wait() {
    LOG_DEBUG("ShmQueue wait");
    const auto ftxVal = futex.load(std::memory_order_acquire);
    flag.store(true, std::memory_order_release);
    utils::futexWait(futex, ftxVal);
    flag.store(false, std::memory_order_release);
  }
};

} // namespace hft

#endif // HFT_COMMON_SHMQUEUE_HPP