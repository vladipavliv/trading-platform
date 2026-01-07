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
#include "utils/sync_utils.hpp"

namespace hft {

template <typename Transport>
class ShmReactor {
public:
  explicit ShmReactor(ShmLayout *layout, ReactorType type) : type_{type}, layout_{layout} {}

  void add(Transport *t) {
    if (type_ == ReactorType::Server && t->type() == ShmTransportType::Upstream) {
      transport_ = t;
    } else if (type_ == ReactorType::Client && t->type() == ShmTransportType::Downstream) {
      transport_ = t;
    }
  }

  void remove(Transport *t) {
    if (t == transport_) {
      transport_ = nullptr;
    }
  }

  void run() {
    LOG_DEBUG_SYSTEM("ShmReactor run");
    if (transport_ == nullptr) {
      throw std::runtime_error("Unable to start, transport is not initialized");
    }
    running_.store(true, std::memory_order_release);
    auto &ftx = localFtx();
    auto &waitFlag = localWaitingFlag();

    while (running_) {
      LOG_DEBUG("Reactor cycle");
      if (transportClosed()) {
        break;
      }

      if (transport_->tryDrain() > 0) {
        continue;
      }

      waitFlag.store(true, std::memory_order_seq_cst);
      uint32_t ftxVal = ftx.load(std::memory_order_acquire);

      if (transport_->tryDrain() > 0) {
        waitFlag.store(false, std::memory_order_relaxed);
        continue;
      }

      if (transportClosed()) {
        break;
      }

      utils::hybridWait(ftx, ftxVal);
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

  void notifyClosed(Transport *t) {
    if (transport_ != t) {
      t->acknowledgeClosure();
    } else {
      notifyLocal();
    }
  }

  void notifyRemote() {
    if (isRemoteWaiting()) {
      LOG_DEBUG("notify remote");
      auto &ftx = remoteFtx();
      ftx.fetch_add(1, std::memory_order_release);
      utils::futexWake(ftx);
    }
  }

  inline bool isRunning() const { return running_; }

private:
  inline bool transportClosed() {
    LOG_DEBUG("transportClosed {} {}", static_cast<void *>(transport_), transport_->isClosed());
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

  inline bool isRemoteWaiting() const {
    return type_ == ReactorType::Server ? layout_->downstreamWaiting.load(std::memory_order_acquire)
                                        : layout_->upstreamWaiting.load(std::memory_order_acquire);
  }

private:
  const ReactorType type_;

  ShmLayout *layout_{nullptr};

  Transport *transport_{nullptr};
  Atomic<bool> running_{false};
};

} // namespace hft

#endif // HFT_COMMON_SHMREACTOR_HPP