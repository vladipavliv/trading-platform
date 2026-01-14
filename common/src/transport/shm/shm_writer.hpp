/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_SHMWRITER_HPP
#define HFT_COMMON_SHMWRITER_HPP

#include "constants.hpp"
#include "container_types.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"
#include "shm_queue.hpp"
#include "transport/async_transport.hpp"
#include "utils/spin_wait.hpp"
#include "utils/sync_utils.hpp"
#include "utils/time_utils.hpp"

namespace hft {

class ShmWriter {
public:
  using RxHandler = std::move_only_function<void(IoResult, size_t)>;

  explicit ShmWriter(ShmQueue &queue) : queue_{queue} {}

  ShmWriter(ShmWriter &&other) noexcept
      : queue_{other.queue_}, running_(other.running_.load(std::memory_order_acquire)) {
    other.running_.store(false, std::memory_order_release);
  }

  ShmWriter(const ShmWriter &) = delete;

  ~ShmWriter() { close(); }

  void asyncTx(CByteSpan buffer, RxHandler clb, uint32_t retry = SPIN_RETRIES_HOT) {
    using namespace utils;
    SpinWait waiter{retry};
    while (!queue_.queue.write(buffer.data(), buffer.size()) &&
           running_.load(std::memory_order_acquire)) {
      if (!++waiter) {
        LOG_ERROR("would block");
        clb(IoResult::Error, 0);
        return;
      }
    }
    queue_.notify();
    clb(IoResult::Ok, buffer.size());
  }

  void close() noexcept {
    LOG_DEBUG("ShmWriter close");
    running_.store(false, std::memory_order_release);
  }

private:
  ShmQueue &queue_;
  AtomicBool running_{true};
};

} // namespace hft

#endif // HFT_COMMON_SHMWRITER_HPP