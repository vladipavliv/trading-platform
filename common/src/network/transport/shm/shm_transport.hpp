/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_SHMTRANSPORT_HPP
#define HFT_COMMON_SHMTRANSPORT_HPP

#include "network/async_transport.hpp"
#include "shm_reactor.hpp"
#include "shm_ring_buffer.hpp"
#include "types.hpp"

namespace hft {

/**
 * @brief
 */
class ShmTransport {
public:
  using Reactor = ShmReactor<ShmTransport>;

  using RxHandler = std::function<void(IoResult, size_t)>;

  ShmTransport(ShmRingBuffer &buffer, Reactor &reactor) : buffer_{buffer}, reactor_{reactor} {
    reactor_.add(this);
  }

  ~ShmTransport() { reactor_.remove(this); }

  template <typename Callable>
  void asyncRx(ByteSpan buf, Callable &&clb) {
    rxBuf_ = buf;
    rxCb_ = std::move(clb);
    rxArmed_ = true;

    tryDrain();
  }

  template <typename Callable>
  void asyncTx(ByteSpan buffer, Callable &&clb) {
    if (buffer_.write(buffer.data(), buffer.size())) {
      reactor_.notify();
      clb(IoResult::Ok, buffer.size());
    } else {
      LOG_ERROR_SYSTEM("Shm ringbuffer is full, dropping message");
      clb(IoResult::Error, 0);
    }
  }

  void close() noexcept { rxArmed_ = false; }

  inline size_t tryDrain() noexcept {
    if (!rxArmed_) {
      return 0;
    }

    const size_t bytes = buffer_.read(rxBuf_.data(), rxBuf_.size());
    if (bytes == 0) {
      return 0;
    }

    rxArmed_ = false;
    const auto clb = std::move(rxCb_);
    clb(IoResult::Ok, bytes);

    return bytes;
  }

private:
  ShmRingBuffer &buffer_;
  Reactor &reactor_;

  bool rxArmed_{false};
  ByteSpan rxBuf_;
  RxHandler rxCb_;
};

} // namespace hft

#endif // HFT_COMMON_SHMTRANSPORT_HPP