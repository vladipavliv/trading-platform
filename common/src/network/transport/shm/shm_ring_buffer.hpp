/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_SHMRINGBUFFER_HPP
#define HFT_COMMON_SHMRINGBUFFER_HPP

#include <algorithm>
#include <atomic>
#include <cstdint>

#include "network/async_transport.hpp"
#include "primitive_types.hpp"

namespace hft {

struct ShmRingBuffer {
  using Index = uint32_t;

  static constexpr uint32_t BUFFER_SIZE = 1024 * 1024;
  static constexpr uint32_t MASK = BUFFER_SIZE - 1;

  static_assert((BUFFER_SIZE & (BUFFER_SIZE - 1)) == 0);

  alignas(64) std::atomic<Index> head{0};
  alignas(64) std::atomic<Index> tail{0};
  alignas(64) uint8_t data[BUFFER_SIZE];

  Index cached_head{0};
  Index cached_tail{0};

  bool write(const uint8_t *buf, uint32_t len) noexcept {
    const Index t = tail.load(std::memory_order_relaxed);
    const Index h = cached_head;

    if (len > BUFFER_SIZE - (t - h) - 1) [[unlikely]] {
      cached_head = head.load(std::memory_order_acquire);
      if (len > BUFFER_SIZE - (t - cached_head) - 1) {
        return false;
      }
    }

    const Index offset = t & MASK;
    const Index first_part = std::min(len, BUFFER_SIZE - offset);

    std::memcpy(data + offset, buf, first_part);
    if (first_part < len) [[unlikely]] {
      std::memcpy(data, buf + first_part, len - first_part);
    }

    tail.store(t + len, std::memory_order_release);
    return true;
  }

  uint32_t read(uint8_t *buf, Index maxLen) noexcept {
    const Index h = head.load(std::memory_order_relaxed);
    Index t = cached_tail;

    Index avail = t - h;
    if (avail == 0) [[unlikely]] {
      t = tail.load(std::memory_order_acquire);
      cached_tail = t;
      avail = t - h;
      if (avail == 0)
        return 0;
    }

    const Index toRead = std::min(avail, maxLen);
    const Index offset = h & MASK;
    const Index first_part = std::min(toRead, BUFFER_SIZE - offset);

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