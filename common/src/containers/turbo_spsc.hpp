/**
 * @author Vladimir Pavliv
 * @date 2026-01-24
 */

#ifndef HFT_COMMON_TURBOSPSC_HPP
#define HFT_COMMON_TURBOSPSC_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include "constants.hpp"
#include "primitive_types.hpp"

namespace hft {

template <size_t Capacity = LFQ_CAPACITY>
class TurboSPSC {
  static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");

public:
  static constexpr size_t BUFFER_SIZE = Capacity * 64;
  static constexpr size_t MASK = BUFFER_SIZE - 1;
  static constexpr size_t MAX_BATCH_SIZE = 64;

  TurboSPSC() noexcept = default;
  TurboSPSC(const TurboSPSC &) = delete;
  TurboSPSC &operator=(const TurboSPSC &) = delete;

  template <typename T>
  [[nodiscard]] bool write(const T &msg) noexcept {
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable for memcpy");
    constexpr size_t size = sizeof(T);

    const size_t currentTail = tail_.load(std::memory_order_relaxed);

    if (currentTail + size > headCache_ + BUFFER_SIZE) {
      headCache_ = head_.load(std::memory_order_acquire);
      if (currentTail + size > headCache_ + BUFFER_SIZE) {
        return false;
      }
    }

    const size_t writePos = currentTail & MASK;
    const auto *src = reinterpret_cast<const uint8_t *>(&msg);

    if (writePos + size <= BUFFER_SIZE) {
      std::memcpy(buffer_ + writePos, src, size);
    } else {
      const size_t first = BUFFER_SIZE - writePos;
      std::memcpy(buffer_ + writePos, src, first);
      std::memcpy(buffer_, src + first, size - first);
    }

    tail_.store(currentTail + size, std::memory_order_release);
    return true;
  }

  template <typename T>
  [[nodiscard]] bool read(T &msg) noexcept {
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable for memcpy");
    constexpr size_t size = sizeof(T);

    if (readCursor_ + size > tailCache_) {
      head_.store(readCursor_, std::memory_order_release);
      tailCache_ = tail_.load(std::memory_order_acquire);
      if (readCursor_ + size > tailCache_) {
        return false;
      }
    }

    const size_t readPos = readCursor_ & MASK;
    auto *dst = reinterpret_cast<uint8_t *>(&msg);

    if (readPos + size <= BUFFER_SIZE) {
      std::memcpy(dst, buffer_ + readPos, size);
    } else {
      const size_t first = BUFFER_SIZE - readPos;
      std::memcpy(dst, buffer_ + readPos, first);
      std::memcpy(dst + first, buffer_, size - first);
    }

    readCursor_ += size;
    return true;
  }

  template <typename T>
  [[nodiscard]] size_t writeBatch(const T *msgs, size_t count) noexcept {
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable for memcpy");
    if (count == 0) {
      return 0;
    }
    if (count > MAX_BATCH_SIZE) {
      count = MAX_BATCH_SIZE;
    }

    constexpr size_t size = sizeof(T);
    const size_t currentTail = tail_.load(std::memory_order_relaxed);

    size_t available = headCache_ + BUFFER_SIZE - currentTail;
    if (available < size) {
      headCache_ = head_.load(std::memory_order_acquire);
      available = headCache_ + BUFFER_SIZE - currentTail;
      if (available < size) {
        return 0;
      }
    }

    size_t n = available / size;
    if (n > count) {
      n = count;
    }

    copyIntoRing(currentTail & MASK, msgs, n);

    tail_.store(currentTail + n * size, std::memory_order_release);
    return n;
  }

  template <typename T>
  [[nodiscard]] size_t readBatch(T *msgs, size_t count) noexcept {
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable for memcpy");
    if (count == 0) {
      return 0;
    }
    if (count > MAX_BATCH_SIZE) {
      count = MAX_BATCH_SIZE;
    }

    constexpr size_t size = sizeof(T);

    if (readCursor_ + size > tailCache_) {
      head_.store(readCursor_, std::memory_order_release);
      tailCache_ = tail_.load(std::memory_order_acquire);
      if (readCursor_ + size > tailCache_) {
        return 0;
      }
    }

    size_t available = tailCache_ - readCursor_;
    size_t n = available / size;
    if (n > count) {
      n = count;
    }

    copyFromRing(readCursor_ & MASK, msgs, n);

    readCursor_ += n * size;
    return n;
  }

  [[nodiscard]] bool empty() const noexcept {
    return readCursor_ == tail_.load(std::memory_order_acquire);
  }

  [[nodiscard]] size_t size() const noexcept {
    const size_t tail = tail_.load(std::memory_order_acquire);
    const size_t head = head_.load(std::memory_order_acquire);
    return tail - head;
  }

  static constexpr size_t capacity() noexcept { return BUFFER_SIZE; }

private:
  template <typename T>
  void copyIntoRing(size_t writePos, const T *src, size_t n) noexcept {
    const size_t bytes = n * sizeof(T);
    if (writePos + bytes <= BUFFER_SIZE) {
      std::memcpy(buffer_ + writePos, src, bytes);
    } else {
      const size_t firstBytes = BUFFER_SIZE - writePos;
      const size_t firstCount = firstBytes / sizeof(T);
      std::memcpy(buffer_ + writePos, src, firstCount * sizeof(T));
      std::memcpy(buffer_, src + firstCount, (n - firstCount) * sizeof(T));
    }
  }

  template <typename T>
  void copyFromRing(size_t readPos, T *dst, size_t n) noexcept {
    const size_t bytes = n * sizeof(T);
    if (readPos + bytes <= BUFFER_SIZE) {
      std::memcpy(dst, buffer_ + readPos, bytes);
    } else {
      const size_t firstBytes = BUFFER_SIZE - readPos;
      const size_t firstCount = firstBytes / sizeof(T);
      std::memcpy(dst, buffer_ + readPos, firstCount * sizeof(T));
      std::memcpy(dst + firstCount, buffer_, (n - firstCount) * sizeof(T));
    }
  }

  alignas(64) std::atomic<size_t> tail_{0};
  alignas(64) size_t headCache_{0};

  alignas(64) std::atomic<size_t> head_{0};
  alignas(64) size_t tailCache_{0};

  alignas(64) size_t readCursor_{0};

  alignas(64) uint8_t buffer_[BUFFER_SIZE];
};

} // namespace hft

#endif // HFT_COMMON_TURBOSPSC_HPP