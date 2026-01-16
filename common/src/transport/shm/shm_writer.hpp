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
#include "shm_ptr.hpp"
#include "shm_queue.hpp"
#include "transport/async_transport.hpp"
#include "utils/spin_wait.hpp"
#include "utils/sync_utils.hpp"
#include "utils/time_utils.hpp"

namespace hft {

class ShmWriter {
public:
  using RxHandler = std::move_only_function<void(IoResult, size_t)>;

  ShmWriter(CRef<String> name, bool createNew) : shm_{name, createNew} {}
  ShmWriter(ShmWriter &&other) noexcept : shm_{std::move(other.shm_)} {}
  ~ShmWriter() = default;

  ShmWriter(const ShmWriter &) = delete;

  void asyncTx(CByteSpan buffer, RxHandler clb, uint32_t retry = SPIN_RETRIES_HOT) {
    using namespace utils;
    SpinWait waiter{retry};
    while (!shm_->queue.write(buffer.data(), buffer.size())) {
      if (!++waiter) {
        LOG_ERROR("would block");
        clb(IoResult::Error, 0);
        return;
      }
    }
    shm_->notify();
    clb(IoResult::Ok, buffer.size());
  }

private:
  ShmUPtr<ShmQueue> shm_;
};

} // namespace hft

#endif // HFT_COMMON_SHMWRITER_HPP