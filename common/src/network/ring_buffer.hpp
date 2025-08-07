/**
 * @author Vladimir Pavliv
 * @date 2025-03-22
 */

#ifndef HFT_COMMON_RINGBUFFER_HPP
#define HFT_COMMON_RINGBUFFER_HPP

#include <memory>

#include "boost_types.hpp"
#include "constants.hpp"
#include "types.hpp"

namespace hft {

/**
 * @brief Wrapper for a ring buffer to use with boost sockets
 */
class RingBuffer {
  static constexpr size_t MIN_READ_CAPACITY = 512;

public:
  explicit RingBuffer(size_t capacity = BUFFER_SIZE) : capacity_{capacity} {
    if (capacity < MIN_READ_CAPACITY) {
      throw std::runtime_error("Invalid ring buffer capacity");
    }
    buffer_.resize(capacity_);
  }

  /**
   * @brief Returns write buffer
   * @details read/write are not used in the interface to avoid confusion
   * whos reading and whos writing. Socket is reading from network but writing to a buffer
   */
  inline auto buffer() -> MutableBuffer {
    rotate();
    return MutableBuffer(buffer_.data() + head_, capacity_ - head_);
  }

  inline auto data() -> ByteSpan {
    assert(tail_ <= head_);
    return ByteSpan(buffer_.data() + tail_, head_ - tail_);
  }

  inline void commitWrite(size_t bytes) {
    assert(head_ + bytes <= capacity_);
    head_ += bytes;
  }

  inline void commitRead(size_t bytes) {
    assert(tail_ + bytes <= head_);
    tail_ += bytes;
    if (tail_ == head_) {
      reset();
    }
  }

  inline void reset() { tail_ = head_ = 0; }

private:
  inline void rotate() {
    if (tail_ > 0 && capacity_ - head_ < MIN_READ_CAPACITY) {
      std::memmove(buffer_.data(), buffer_.data() + tail_, head_ - tail_);
      head_ = head_ - tail_;
      tail_ = 0;
    }
  }

private:
  size_t head_{0};
  size_t tail_{0};
  size_t capacity_{0};
  ByteBuffer buffer_;
};

} // namespace hft

#endif // HFT_COMMON_RINGBUFFER_HPP
