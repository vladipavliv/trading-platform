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
struct ShmRingBuffer {
  static constexpr size_t BUFFER_SIZE = 1024 * 1024;
  static_assert((BUFFER_SIZE & (BUFFER_SIZE - 1)) == 0,
                "BUFFER_SIZE must be power of 2 for fast masking");

  alignas(64) std::atomic<size_t> head{0};
  alignas(64) std::atomic<size_t> tail{0};
  uint8_t data[BUFFER_SIZE];

  static constexpr size_t MASK = BUFFER_SIZE - 1;

  size_t available() const noexcept {
    size_t h = head.load(std::memory_order_acquire);
    size_t t = tail.load(std::memory_order_acquire);
    return (t - h) & MASK;
  }

  size_t space() const noexcept { return BUFFER_SIZE - available() - 1; }

  bool write(const uint8_t *buf, size_t len) noexcept {
    if (len > space())
      return false;

    size_t t = tail.load(std::memory_order_relaxed);

    size_t first = std::min(len, BUFFER_SIZE - (t & MASK));
    std::memcpy(data + (t & MASK), buf, first);

    if (len > first) {
      std::memcpy(data, buf + first, len - first);
    }

    tail.store(t + len, std::memory_order_release);
    return true;
  }

  size_t read(uint8_t *buf, size_t maxLen) noexcept {
    size_t avail = available();
    if (avail == 0)
      return 0;

    size_t toRead = std::min(avail, maxLen);
    size_t h = head.load(std::memory_order_relaxed);

    size_t first = std::min(toRead, BUFFER_SIZE - (h & MASK));
    std::memcpy(buf, data + (h & MASK), first);

    if (toRead > first) {
      std::memcpy(buf + first, data, toRead - first);
    }

    head.store(h + toRead, std::memory_order_release);
    return toRead;
  }
};

} // namespace hft

#endif // HFT_COMMON_SHMRINGBUFFER_HPP