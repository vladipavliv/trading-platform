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
#include "types/metadata_types.hpp"
#include "utils/sync_utils.hpp"
#include "utils/thread_utils.hpp"

namespace hft {

template <typename TransportT>
class ShmReactor {
public:
  explicit ShmReactor(ShmLayout *layout, ReactorType type, SystemBus &bus)
      : type_{type}, layout_{layout}, bus_{bus} {}

  void add(TransportT *t) {
    if (type_ == ReactorType::Server && t->type() == ShmTransportType::Upstream) {
      transport_ = t;
    } else if (type_ == ReactorType::Client && t->type() == ShmTransportType::Downstream) {
      transport_ = t;
    }
  }

  void remove(TransportT *t) {
    if (t == transport_) {
      transport_ = nullptr;
    }
  }

  void run() {
    LOG_DEBUG("ShmReactor run");
    if (transport_ == nullptr) {
      throw std::runtime_error("Unable to start, transport is not initialized");
    }
    counters_.id = utils::getCoreId();

    running_.store(true, std::memory_order_release);
    auto &ftx = localFtx();
    auto &waitFlag = localWaitingFlag();

    uint64_t cycles = 0;
    while (running_) {
      if (++cycles > 3000000) {
        counters_.ctxSwitches = utils::checkSwitches();
        bus_.post(counters_);
        cycles = 0;
      }

      if (transportClosed()) {
        break;
      }

      uint64_t drainStart = __rdtsc();
      auto drained = transport_->tryDrain();
      auto delta = __rdtsc() - drainStart;
      counters_.maxDrain = std::max(counters_.maxDrain, (uint64_t)delta);

      if (drained > 0) {
        continue;
      }

      uint32_t ftxVal = ftx.load(std::memory_order_acquire);
      waitFlag.store(true, std::memory_order_release);

      drainStart = __rdtsc();
      drained = transport_->tryDrain();
      delta = __rdtsc() - drainStart;
      counters_.maxDrain = std::max(counters_.maxDrain, (uint64_t)delta);

      if (drained > 0) {
        continue;
      }

      if (transportClosed()) {
        break;
      }

      auto waitRes = utils::hybridWait(ftx, ftxVal);
      if (waitRes == 10'000) {
        counters_.futexWait++;
      } else {
        counters_.pause += waitRes;
      }
      waitFlag.store(false, std::memory_order_relaxed);
    }
    LOG_DEBUG("ShmReactor stopped");
  }

  void stop() {
    LOG_DEBUG("Stopping ShmReactor");
    running_.store(false, std::memory_order_release);
    notifyLocal();
    notifyRemote();
  }

  void notifyLocal() {
    LOG_DEBUG("notify local");
    auto &ftx = localFtx();
    ftx.fetch_add(1, std::memory_order_release);
    utils::futexWake(ftx);
  }

  void notifyClosed(TransportT *t) {
    if (transport_ != t) {
      t->acknowledgeClosure();
    } else {
      notifyLocal();
    }
  }

  void notifyRemote() {
    auto &w = getRemoteWaiting();
    if (w.load(std::memory_order_acquire)) {
      counters_.futexWake++;
      auto &ftx = remoteFtx();
      auto val = ftx.fetch_add(1, std::memory_order_release);
      LOG_TRACE("notify remote {}", val);
      utils::futexWake(ftx);
    }
  }

  inline bool isRunning() const { return running_; }

private:
  inline bool transportClosed() {
    LOG_TRACE("transportClosed {} {}", static_cast<void *>(transport_), transport_->isClosed());
    if (transport_ != nullptr && transport_->isClosed()) {
      transport_->acknowledgeClosure();
      transport_ = nullptr;
      return true;
    }
    return transport_ == nullptr;
  }

  inline auto localFtx() const -> Atomic<uint32_t> & {
    return type_ == ReactorType::Server ? layout_->upstreamFtx : layout_->downstreamFtx;
  }

  inline auto remoteFtx() const -> Atomic<uint32_t> & {
    return type_ != ReactorType::Server ? layout_->upstreamFtx : layout_->downstreamFtx;
  }

  inline auto localWaitingFlag() const -> Atomic<bool> & {
    return type_ == ReactorType::Server ? layout_->upstreamWaiting : layout_->downstreamWaiting;
  }

  inline AtomicBool &getRemoteWaiting() {
    return type_ == ReactorType::Server ? layout_->downstreamWaiting : layout_->upstreamWaiting;
  }

  inline bool isRemoteWaiting() const { return getRemoteWaiting().load(std::memory_order_acquire); }

private:
  const ReactorType type_;
  SystemBus &bus_;

  ShmLayout *layout_{nullptr};

  TransportT *transport_{nullptr};
  Atomic<bool> running_{false};

  ThreadCounters counters_;
};

} // namespace hft

#endif // HFT_COMMON_SHMREACTOR_HPP
