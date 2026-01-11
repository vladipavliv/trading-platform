/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_SHMREACTOR_HPP
#define HFT_COMMON_SHMREACTOR_HPP

#include "primitive_types.hpp"
#include "shm_drainable.hpp"
#include "shm_layout.hpp"
#include "shm_types.hpp"
#include "utils/profiling_utils.hpp"
#include "utils/sync_utils.hpp"
#include "utils/thread_utils.hpp"

namespace hft {

constexpr size_t COLLECT_STATS_CYCLES = 10000000;

template <typename TransportT>
class ShmReactor {
public:
  explicit ShmReactor(ShmLayout &layout, ReactorType type, SystemBus &bus)
      : type_{type}, layout_{layout}, bus_{bus} {}

  void add(TransportT *t) {
    transports_.push_back(t);
    if (type_ == ReactorType::Server && t->type() == ShmTransportType::Upstream) {
      transport_ = t;
    } else if (type_ == ReactorType::Client && t->type() == ShmTransportType::Downstream) {
      transport_ = t;
    }
  }

  void remove(TransportT *t) {
    transports_.erase(std::remove(transports_.begin(), transports_.end(), t), transports_.end());
    if (t == transport_) {
      transport_ = nullptr;
    }
  }

  void run() {
    using namespace utils;
    LOG_DEBUG("ShmReactor run");
    if (transport_ == nullptr) {
      throw std::runtime_error("Unable to start, transport is not initialized");
    }

    running_.store(true, std::memory_order_release);
    auto &ftx = localFtx();
    auto &flag = localFlag();

    uint64_t statCycles = 0;
    uint64_t emptyCycles = 0;
    while (running_.load(std::memory_order_acquire)) {
      const size_t drained = transport_->tryDrain();
      // const size_t drained = PROFILE_EXEC(transport_->tryDrain(), pfData_.maxCall);
      if (drained > 0) {
        // pfData_.waitSpins += emptyCycles;
        emptyCycles = 0;
        continue;
      }

      if (++emptyCycles < BUSY_WAIT_CYCLES) {
        asm volatile("pause" ::: "memory");
        continue;
      }

      if (!running_.load(std::memory_order_acquire)) {
        continue;
      }

      emptyCycles = 0;
      const auto ftxVal = ftx.load(std::memory_order_acquire);
      flag.store(true, std::memory_order_release);
      futexWait(ftx, ftxVal);
      flag.store(false, std::memory_order_release);
    }
    std::for_each(transports_.begin(), transports_.end(),
                  [](TransportT *t) { t->acknowledgeClosure(); });
    LOG_INFO_SYSTEM("ShmReactor stopped");
  }

  void stop() {
    LOG_DEBUG("Stopping ShmReactor");
    running_.store(false, std::memory_order_release);
    utils::futexWake(localFtx());
  }

  void notifyRemote() {
    auto &flag = remoteFlag();
    auto &ftx = remoteFtx();

    if (flag.load(std::memory_order_acquire)) {
      ftx.store(++ftxWake_, std::memory_order_release);
      flag.store(false, std::memory_order_release);
      utils::futexWake(ftx);
    }
  }

  inline bool isRunning() const { return running_; }

private:
  inline auto localFtx() const -> Atomic<uint32_t> & {
    return type_ == ReactorType::Server ? layout_.upstreamFtx : layout_.downstreamFtx;
  }

  inline auto remoteFtx() const -> Atomic<uint32_t> & {
    return type_ != ReactorType::Server ? layout_.upstreamFtx : layout_.downstreamFtx;
  }

  inline auto localFlag() const -> Atomic<bool> & {
    return type_ == ReactorType::Server ? layout_.upstreamWaiting : layout_.downstreamWaiting;
  }

  inline auto remoteFlag() const -> Atomic<bool> & {
    return type_ != ReactorType::Server ? layout_.upstreamWaiting : layout_.downstreamWaiting;
  }

private:
  const ReactorType type_;
  SystemBus &bus_;

  ShmLayout &layout_;

  TransportT *transport_{nullptr};
  Vector<TransportT *> transports_;

  AtomicBool running_{false};
  uint32_t ftxWake_{0};
};

} // namespace hft

#endif // HFT_COMMON_SHMREACTOR_HPP
