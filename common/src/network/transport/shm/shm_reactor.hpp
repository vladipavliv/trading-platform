/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_SHMREACTOR_HPP
#define HFT_COMMON_SHMREACTOR_HPP

#include <atomic>

#include "shm_drainable.hpp"
#include "shm_layout.hpp"
#include "shm_types.hpp"

namespace hft {

template <typename Transport>
class ShmReactor {
public:
  explicit ShmReactor(ShmLayout *layout, ReactorType type) : type_{type}, layout_{layout} {}

  void add(Transport *t) { transports_.push_back(t); }

  void remove(Transport *t) {
    transports_.erase(std::remove(transports_.begin(), transports_.end(), t), transports_.end());
  }

  void run() {
    LOG_INFO_SYSTEM("ShmReactor run");

    running_ = true;
    size_t spins = 0;
    auto &ftx = localFtx();
    while (running_) {
      uint32_t currentCounter = ftx.load(std::memory_order_acquire);
      LOG_DEBUG("Reactor loop {} {}", seqCounter_, currentCounter);
      while (running_ && currentCounter == seqCounter_) {
        if (spins < BUSY_WAIT_CYCLES) {
          asm volatile("pause" ::: "memory");
          ++spins;
        } else {
          utils::futexWait(ftx, currentCounter);
          // std::this_thread::yield();
          spins = 0;
        }
        currentCounter = ftx.load(std::memory_order_acquire);
      }
      if (!running_) {
        return;
      }
      for (auto *t : transports_) {
        if (type_ == ReactorType::Server && t->type() == ShmTransportType::Upstream) {
          t->tryDrain();
        } else if (type_ == ReactorType::Client && t->type() != ShmTransportType::Upstream) {
          t->tryDrain();
        }
      }

      seqCounter_ = currentCounter;
      spins = 0;
    }
  }

  void stop() {
    running_ = false;
    utils::futexWake(localFtx());
    utils::futexWake(remoteFtx());
  }

  void notify() {
    LOG_DEBUG("notify");
    auto &ftx = remoteFtx();
    ftx.fetch_add(1, std::memory_order_release);
    utils::futexWake(ftx);
  }

  inline bool running() const { return running_; }

private:
  inline auto localFtx() const -> std::atomic<uint32_t> & {
    return type_ == ReactorType::Server ? layout_->upstreamFtx : layout_->downstreamFtx;
  }

  inline auto remoteFtx() const -> std::atomic<uint32_t> & {
    return type_ != ReactorType::Server ? layout_->upstreamFtx : layout_->downstreamFtx;
  }

private:
  const ReactorType type_;

  uint32_t seqCounter_{0};

  ShmLayout *layout_;

  std::vector<Transport *> transports_;
  std::atomic<bool> running_{false};
};

} // namespace hft

#endif // HFT_COMMON_SHMREACTOR_HPP