/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_SHMREADER_HPP
#define HFT_COMMON_SHMREADER_HPP

#include "bus/system_bus.hpp"
#include "config/config.hpp"
#include "container_types.hpp"
#include "containers/sequenced_spsc.hpp"
#include "io_result.hpp"
#include "primitive_types.hpp"
#include "shm_queue.hpp"
#include "utils/spin_wait.hpp"
#include "utils/thread_utils.hpp"

namespace hft {

class ShmReader {
public:
  using RxHandler = std::move_only_function<void(IoResult, size_t)>;

  explicit ShmReader(ShmQueue &queue, ErrorBus bus) : queue_{queue}, bus_{std::move(bus)} {}

  ShmReader(ShmReader &&other) noexcept
      : queue_(other.queue_), bus_(std::move(other.bus_)), buf_(std::move(other.buf_)),
        clb_(std::move(other.clb_)), running_(other.running_.load(std::memory_order_acquire)) {
    runner_ = std::move(other.runner_);
    other.running_.store(false, std::memory_order_release);
  }

  ShmReader(const ShmReader &) = delete;

  ~ShmReader() {
    LOG_DEBUG("ShmReader running:{}", running_.load());
    if (running_) {
      close();
    }
  }

  void asyncRx(ByteSpan buf, RxHandler clb) {
    if (running_) {
      return;
    }
    LOG_INFO_SYSTEM("Starting shm reader");
    buf_ = buf;
    clb_ = std::move(clb);
    runner_ = std::jthread([this]() {
      try {
        utils::setThreadRealTime();
        const auto coreId = Config::get_optional<CoreId>("cpu.core_network");
        if (coreId.has_value()) {
          utils::pinThreadToCore(*coreId);
        }
        running_.store(true, std::memory_order_release);
        readLoop();
      } catch (const std::exception &ex) {
        LOG_ERROR_SYSTEM("Exception in ShmReader {}", ex.what());
      } catch (...) {
        LOG_ERROR_SYSTEM("Unknown exception in ShmReader");
      }
    });
  }

  void close() {
    LOG_DEBUG("ShmReader close");
    running_.store(false, std::memory_order_release);
    utils::futexWake(queue_.futex);
  }

private:
  void readLoop() {
    SpinWait waiter;
    while (running_.load(std::memory_order_acquire)) {
      const size_t bytes = queue_.queue.read(buf_.data(), buf_.size());
      if (bytes != 0) {
        clb_(IoResult::Ok, bytes);
        continue;
      }
      if (++waiter) {
        continue;
      }
      if (!running_.load(std::memory_order_acquire)) {
        break;
      }
      waiter.reset();
      queue_.wait();
    }
  }

private:
  ErrorBus bus_;
  ShmQueue &queue_;

  ByteSpan buf_;
  RxHandler clb_;

  AtomicBool running_{false};
  std::jthread runner_;
};

} // namespace hft

#endif // HFT_COMMON_SHMREADER_HPP