/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_SHMTRANSPORT_HPP
#define HFT_COMMON_SHMTRANSPORT_HPP

#include <functional>

#include "constants.hpp"
#include "logging.hpp"
#include "network/async_transport.hpp"
#include "primitive_types.hpp"
#include "shm_reactor.hpp"
#include "shm_ring_buffer.hpp"
#include "shm_types.hpp"
#include "utils/sync_utils.hpp"

namespace hft {

/**
 * @brief
 */
class ShmTransport {
public:
  using RxHandler = std::move_only_function<void(IoResult, size_t)>;

  ShmTransport(ShmTransportType type, ShmRingBuffer &buffer, ShmReactor<ShmTransport> &reactor)
      : type_{type}, buffer_{buffer}, reactor_{reactor} {
    reactor_.add(this);
  }

  ShmTransport(ShmTransport &&other) noexcept
      : type_{other.type_}, buffer_{other.buffer_}, reactor_{other.reactor_},
        rxArmed_{other.rxArmed_.exchange(false)}, rxBuf_{other.rxBuf_},
        rxCb_{std::move(other.rxCb_)} {
    reactor_.remove(&other);
    reactor_.add(this);
    other.closed_.store(true, std::memory_order_release);
    other.deleted_.store(true, std::memory_order_release);
  }

  ShmTransport(const ShmTransport &) = delete;

  ~ShmTransport() {
    LOG_DEBUG("~ShmTransport");
    if (!closed_.load(std::memory_order_acquire)) {
      close();
    }
    reactor_.remove(this);
  }

  void asyncRx(ByteSpan buf, RxHandler clb) {
    LOG_TRACE("asyncRx {}", buf.size());
    if (closed_) {
      LOG_ERROR("Transport is already closed");
      clb(IoResult::Closed, 0);
      return;
    }
    rxBuf_ = buf;
    rxCb_ = std::move(clb);
    rxArmed_.store(true, std::memory_order_release);
  }

  void asyncTx(ByteSpan buffer, auto &&clb) {
    LOG_TRACE("asyncTx {}", buffer.size());
    if (closed_) {
      LOG_ERROR("Transport is already closed");
      clb(IoResult::Closed, 0);
      return;
    }
    size_t cycles = 0;
    while (!closed_ && !buffer_.write(buffer.data(), buffer.size())) {
      asm volatile("pause" ::: "memory");
      if (++cycles > BUSY_WAIT_CYCLES) {
        LOG_ERROR("would block");
        clb(IoResult::WouldBlock, 0);
        return;
      }
    }
    if (closed_) {
      return;
    }
    reactor_.notifyRemote();
    clb(IoResult::Ok, buffer.size());
  }

  void close() noexcept {
    LOG_DEBUG("ShmTransport close");
    if (closed_.exchange(true, std::memory_order_release) || !reactor_.isRunning()) {
      LOG_DEBUG("already closed");
      return;
    }
    LOG_DEBUG("Notify reactor {} {}", static_cast<void *>(this), closed_.load());
    reactor_.notifyClosed(this);
    const auto start = std::chrono::steady_clock::now();
    size_t cycles = 0;
    while (!deleted_.load(std::memory_order_acquire)) {
      asm volatile("pause" ::: "memory");
      if (++cycles > BUSY_WAIT_CYCLES) {
        LOG_WARN("Trying to close ShmTransport {}", cycles);
        std::this_thread::yield();
      }
      if (std::chrono::steady_clock::now() - start > Milliseconds(BUSY_WAIT_WALL_MS)) {
        LOG_ERROR_SYSTEM("Failed to properly close ShmTransport");
        break;
      }
    }
  }

  inline size_t tryDrain() noexcept {
    LOG_DEBUG("ShmTransport tryDrain");
    if (closed_) {
      LOG_ERROR("Transport is already closed");
      if (rxArmed_.exchange(false, std::memory_order_acq_rel)) {
        if (rxCb_) {
          auto clb = std::move(rxCb_);
          clb(IoResult::Closed, 0);
        }
      }
      return 0;
    }

    if (!rxArmed_.load(std::memory_order_acquire)) {
      return 0;
    }

    const size_t bytes = buffer_.read(rxBuf_.data(), rxBuf_.size());
    if (bytes == 0) {
      return 0;
    }

    rxArmed_.store(false, std::memory_order_release);

    auto clb = std::move(rxCb_);
    clb(IoResult::Ok, bytes);
    return bytes;
  }

  void acknowledgeClosure() noexcept {
    LOG_DEBUG("acknowledgeClosure");
    deleted_.store(true, std::memory_order_release);
  }

  inline bool isClosed() const { return closed_.load(std::memory_order_acquire); }

  inline ShmTransportType type() const { return type_; }

private:
  const ShmTransportType type_;

  ShmRingBuffer &buffer_;
  ShmReactor<ShmTransport> &reactor_;
  ByteSpan rxBuf_;

  // hot data
  alignas(64) RxHandler rxCb_;
  alignas(64) AtomicBool rxArmed_{false};

  // cold data
  alignas(64) AtomicBool closed_{false};
  alignas(64) AtomicBool deleted_{false};
};

} // namespace hft

#endif // HFT_COMMON_SHMTRANSPORT_HPP