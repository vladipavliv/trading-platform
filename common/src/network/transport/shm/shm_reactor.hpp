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
    LOG_INFO_SYSTEM("ShmReactor run");
    running_ = true;
    while (running_) {
      transport_->tryDrain();
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

  ShmLayout *layout_{nullptr};

  Transport *transport_{nullptr};
  std::atomic<bool> running_{false};
};

} // namespace hft

#endif // HFT_COMMON_SHMREACTOR_HPP