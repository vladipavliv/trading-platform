/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_SHMWRITER_HPP
#define HFT_COMMON_SHMWRITER_HPP

#include "constants.hpp"
#include "container_types.hpp"
#include "functional_types.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"
#include "shm_ptr.hpp"
#include "shm_queue.hpp"
#include "utils/spin_wait.hpp"
#include "utils/sync_utils.hpp"
#include "utils/time_utils.hpp"

namespace hft {

class ShmWriter {
public:
  explicit ShmWriter(CRef<String> name) : closed_{false}, shm_{name} {}
  ShmWriter(ShmWriter &&other) noexcept
      : closed_{other.closed_.load(std::memory_order_acquire)}, shm_{std::move(other.shm_)} {
    other.closed_.store(true, std::memory_order_release);
  }
  ~ShmWriter() = default;

  ShmWriter(const ShmWriter &) = delete;

  auto syncTx(CByteSpan buffer) -> IoResult {
    using namespace utils;
    if (closed_.load(std::memory_order_acquire)) {
      LOG_WARN_SYSTEM("ShmWriter is already closed");
      return IoResult{0, IoStatus::Error};
    }
    SpinWait waiter;
    while (!shm_->queue.write(buffer.data(), buffer.size())) {
      if (shm_->count() < 2) {
        return {0, IoStatus::Closed};
      }
      if (!++waiter) {
        return {0, IoStatus::Error};
      }
    }
    shm_->notify();
    return {(uint32_t)buffer.size(), IoStatus::Ok};
  }

  void asyncTx(CByteSpan buffer, CRefHandler<IoResult> &&clb) {
    using namespace utils;
    if (closed_.load(std::memory_order_acquire)) {
      LOG_WARN_SYSTEM("ShmWriter is already closed");
      return;
    }
    SpinWait waiter;
    while (!shm_->queue.write(buffer.data(), buffer.size())) {
      if (shm_->count() < 2) {
        clb({0, IoStatus::Closed});
        return;
      }
      if (!++waiter) {
        clb({0, IoStatus::Error});
        return;
      }
    }
    shm_->notify();
    clb({(uint32_t)buffer.size(), IoStatus::Ok});
  }

  void close() { closed_.store(true, std::memory_order_release); }

private:
  AtomicBool closed_;
  ShmUPtr<ShmQueue> shm_;
};

} // namespace hft

#endif // HFT_COMMON_SHMWRITER_HPP