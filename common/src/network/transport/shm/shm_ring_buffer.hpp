/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_SHMRINGBUFFER_HPP
#define HFT_COMMON_SHMRINGBUFFER_HPP

#include <atomic>

#include "network/async_transport.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft {

struct ShmRingBuffer {
  static constexpr size_t BUFFER_SIZE = 16 * 1024 * 1024;
  static constexpr size_t MASK = BUFFER_SIZE - 1;

  alignas(64) std::atomic<size_t> head{0};
  alignas(64) std::atomic<size_t> tail{0};
  alignas(64) uint8_t data[BUFFER_SIZE];

  size_t cached_head{0};

  bool write(const uint8_t *buf, size_t len) noexcept {
    const size_t t = tail.load(std::memory_order_relaxed);
    const size_t h = cached_head;

    if (len > BUFFER_SIZE - (t - h) - 1) [[unlikely]] {
      cached_head = head.load(std::memory_order_acquire);
      if (len > BUFFER_SIZE - (t - cached_head) - 1) {
        return false;
      }
    }

    const size_t offset = t & MASK;
    const size_t first_part = std::min(len, BUFFER_SIZE - offset);

    std::memcpy(data + offset, buf, first_part);
    if (first_part < len) [[unlikely]] {
      std::memcpy(data, buf + first_part, len - first_part);
    }

    tail.store(t + len, std::memory_order_release);
    return true;
  }

  size_t cached_tail{0};

  size_t read(uint8_t *buf, size_t maxLen) noexcept {
    const size_t h = head.load(std::memory_order_relaxed);
    size_t t = cached_tail;

    size_t avail = t - h;
    if (avail == 0) [[unlikely]] {
      t = tail.load(std::memory_order_acquire);
      cached_tail = t;
      avail = t - h;
      if (avail == 0)
        return 0;
    }

    const size_t toRead = std::min(avail, maxLen);
    const size_t offset = h & MASK;
    const size_t first_part = std::min(toRead, BUFFER_SIZE - offset);

    std::memcpy(buf, data + offset, first_part);
    if (first_part < toRead) [[unlikely]] {
      std::memcpy(buf + first_part, data, toRead - first_part);
    }

    head.store(h + toRead, std::memory_order_release);
    return toRead;
  }
};

} // namespace hft

#endif // HFT_COMMON_SHMRINGBUFFER_HPP