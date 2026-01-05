/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_SHMREACTOR_HPP
#define HFT_COMMON_SHMREACTOR_HPP

#include <atomic>

#include "shm_drainable.hpp"
#include "shm_layout.hpp"

namespace hft {

template <Drainable Transport>
class ShmReactor {
public:
  explicit ShmReactor(ShmLayout *layout) : layout_{layout} {}

  void add(Transport &t) { transports_.push_back(&t); }

  void remove(Transport &t) {
    transports_.erase(std::remove(transports_.begin(), transports_.end(), &t), transports_.end());
  }

  void run() {
    size_t spins = 0;
    while (running_) {
      uint32_t currentCounter = layout_->futex.load(std::memory_order_acquire);
      while (currentCounter == seqCounter_) {
        if (spins < BUSY_WAIT_CYCLES) {
          asm volatile("pause" ::: "memory");
          ++spins;
        } else {
          utils::futexWait(layout_->futex);
          spins = 0;
        }
        currentCounter = layout_->futex.load(std::memory_order_acquire);
      }
      for (auto *t : transports_) {
        t->tryDrain();
      }
      seqCounter_ = currentCounter;
      spins = 0;
    }
  }

  void stop() {
    running_ = false;
    notify();
  }

  void notify() {
    seqCounter_ = layout_->futex.fetch_add(1, std::memory_order_release);
    utils::futexWake(layout_->futex);
  }

private:
  uint32_t seqCounter_{0};

  ShmLayout *layout_;

  std::vector<class ShmTransport *> transports_;
  std::atomic<bool> running_{true};
};

} // namespace hft

#endif // HFT_COMMON_SHMREACTOR_HPP