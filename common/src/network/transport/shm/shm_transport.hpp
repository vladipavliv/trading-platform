/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_SHMTRANSPORT_HPP
#define HFT_COMMON_SHMTRANSPORT_HPP

#include <functional>

#include "network/async_transport.hpp"
#include "shm_reactor.hpp"
#include "shm_ring_buffer.hpp"
#include "shm_types.hpp"
#include "types.hpp"

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
        rxArmed_{other.rxArmed_}, rxBuf_{other.rxBuf_}, rxCb_{std::move(other.rxCb_)} {

    reactor_.remove(&other);
    reactor_.add(this);
  }

  ShmTransport(const ShmTransport &) = delete;

  ~ShmTransport() {
    close();
    reactor_.remove(this);
  }

  template <typename Callable>
  void asyncRx(ByteSpan buf, Callable &&clb) {
    LOG_DEBUG("asyncRx");
    rxBuf_ = buf;
    rxCb_ = std::move(clb);
    rxArmed_ = true;
  }

  template <typename Callable>
  void asyncTx(ByteSpan buffer, Callable &&clb) {
    LOG_DEBUG("asyncTx");
    if (!reactor_.running()) {
      LOG_ERROR("Reactor is not running");
      return;
    }
    size_t cycles = 0;
    while (!buffer_.write(buffer.data(), buffer.size())) {
      asm volatile("pause" ::: "memory");

      LOG_ERROR_SYSTEM("Shm ringbuffer is full");
      std::this_thread::sleep_for(std::chrono::microseconds(10000));
      if (++cycles > BUSY_WAIT_CYCLES) {
        LOG_ERROR_SYSTEM("Shm ringbuffer is full, dropping message");
        clb(IoResult::Error, 0);
        return;
      }
    }
    reactor_.notify();
    clb(IoResult::Ok, buffer.size());
  }

  void close() noexcept {
    rxArmed_ = false;
    if (rxCb_) {
      auto clb = std::move(rxCb_);
      clb(IoResult::Closed, 0);
    }
  }

  inline size_t tryDrain() noexcept {
    LOG_DEBUG("tryDrain");
    if (!reactor_.running()) {
      LOG_ERROR_SYSTEM("Reactor is not running");
      return 0;
    }

    if (!rxArmed_) {
      LOG_DEBUG("not armed");
      return 0;
    }

    const size_t bytes = buffer_.read(rxBuf_.data(), rxBuf_.size());
    if (bytes == 0) {
      LOG_DEBUG("0 bytes");
      return 0;
    }

    rxArmed_ = false;
    auto clb = std::move(rxCb_);
    clb(IoResult::Ok, bytes);

    return bytes;
  }

  inline ShmTransportType type() const { return type_; }

private:
  const ShmTransportType type_;

  ShmRingBuffer &buffer_;
  ShmReactor<ShmTransport> &reactor_;

  bool rxArmed_{false};
  ByteSpan rxBuf_;
  RxHandler rxCb_;
};

} // namespace hft

#endif // HFT_COMMON_SHMTRANSPORT_HPP