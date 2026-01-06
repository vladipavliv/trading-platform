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

/**
 * @brief Lock-free ring buffer in shared memory
 */
struct FastShmRingBuffer {
  static constexpr size_t BUFFER_SIZE = 16 * 1024 * 1024;
  static constexpr size_t MASK = BUFFER_SIZE - 1;

  alignas(64) std::atomic<size_t> head{0};
  alignas(64) std::atomic<size_t> tail{0};

  alignas(64) mutable size_t cached_head{0};
  alignas(64) mutable size_t cached_tail{0};

  uint8_t data[BUFFER_SIZE];

  inline size_t space_fast(size_t current_tail) const noexcept {
    size_t h = cached_head;
    size_t distance = current_tail - h;

    if (distance >= BUFFER_SIZE) [[unlikely]] {
      h = head.load(std::memory_order_acquire);
      cached_head = h;
      distance = current_tail - h;
    }

    return BUFFER_SIZE - distance - 1;
  }

  inline size_t available_fast(size_t current_head) const noexcept {
    size_t t = cached_tail;
    size_t distance = t - current_head;

    if (distance > BUFFER_SIZE) [[unlikely]] {
      t = tail.load(std::memory_order_acquire);
      cached_tail = t;
      distance = t - current_head;
    }

    return distance;
  }

  bool write(const uint8_t *buf, size_t len) noexcept {
    size_t t = tail.load(std::memory_order_relaxed);

    if (len > space_fast(t)) [[unlikely]]
      return false;

    const size_t offset = t & MASK;
    const size_t first = std::min(len, BUFFER_SIZE - offset);

    __builtin_memcpy(data + offset, buf, first);
    if (len > first) [[unlikely]] {
      __builtin_memcpy(data, buf + first, len - first);
    }

    tail.store(t + len, std::memory_order_release);
    return true;
  }

  size_t read(uint8_t *buf, size_t maxLen) noexcept {
    const size_t h = head.load(std::memory_order_relaxed);
    const size_t avail = available_fast(h);
    if (avail == 0)
      return 0;

    const size_t toRead = std::min(avail, maxLen);
    const size_t offset = h & MASK;
    const size_t first = std::min(toRead, BUFFER_SIZE - offset);

    __builtin_memcpy(buf, data + offset, first);
    if (toRead > first) [[unlikely]] {
      __builtin_memcpy(buf + first, data, toRead - first);
    }

    head.store(h + toRead, std::memory_order_release);
    return toRead;
  }

  bool isEmpty() const noexcept {
    const size_t h = head.load(std::memory_order_acquire);
    const size_t t = tail.load(std::memory_order_acquire);
    return h == t;
  }
};

struct ShmRingBuffer {
  // Power of two is critical for the & MASK trick to work
  static constexpr size_t BUFFER_SIZE = 16 * 1024 * 1024;
  static constexpr size_t MASK = BUFFER_SIZE - 1;

  alignas(64) std::atomic<size_t> head{0};
  alignas(64) std::atomic<size_t> tail{0};

  uint8_t data[BUFFER_SIZE];

  // Helper: How many bytes are actually in the buffer right now
  inline size_t get_occupancy(size_t h, size_t t) const noexcept {
    return t - h; // Works perfectly with unsigned overflow
  }

  bool write(const uint8_t *buf, size_t len) noexcept {
    const size_t t = tail.load(std::memory_order_relaxed);
    const size_t h = head.load(std::memory_order_acquire);

    if (len > (BUFFER_SIZE - get_occupancy(h, t) - 1)) [[unlikely]] {
      return false; // Actually full
    }

    const size_t offset = t & MASK;
    const size_t first_part = std::min(len, BUFFER_SIZE - offset);

    std::memcpy(data + offset, buf, first_part);
    if (len > first_part) [[unlikely]] {
      std::memcpy(data, buf + first_part, len - first_part);
    }

    // Release ensures the data is visible before tail is updated
    tail.store(t + len, std::memory_order_release);
    return true;
  }

  size_t read(uint8_t *buf, size_t maxLen) noexcept {
    const size_t h = head.load(std::memory_order_relaxed);
    const size_t t = tail.load(std::memory_order_acquire);

    const size_t avail = get_occupancy(h, t);
    if (avail == 0)
      return 0;

    const size_t toRead = std::min(avail, maxLen);
    const size_t offset = h & MASK;
    const size_t first_part = std::min(toRead, BUFFER_SIZE - offset);

    std::memcpy(buf, data + offset, first_part);
    if (toRead > first_part) [[unlikely]] {
      std::memcpy(buf + first_part, data, toRead - first_part);
    }

    // Release ensures we are "done" with the memory before updating head
    head.store(h + toRead, std::memory_order_release);
    return toRead;
  }
};

} // namespace hft

#endif // HFT_COMMON_SHMRINGBUFFER_HPP