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

/**
 * @brief Byte stream SPSC queue
 * @details Cached head/tail indices keep the hot path off the shared atomics
 * common case is a relaxed load, a memcpy, and one release store — the atomic
 * exchange only happens when the cached view goes stale (empty/full boundary)
 * Batch write/read amortize the boundary check across up to MAX_BATCH_SIZE
 * elements, and a single flat byte buffer means no per-message overhead or
 * allocation.
 */
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
    return writeImpl(reinterpret_cast<const uint8_t *>(&msg), sizeof(T));
  }

  template <typename T>
  [[nodiscard]] bool read(T &msg) noexcept {
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable for memcpy");
    return readImpl(reinterpret_cast<uint8_t *>(&msg), sizeof(T));
  }

  template <typename T>
  [[nodiscard]] size_t writeBatch(const T *msgs, size_t count) noexcept {
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable for memcpy");
    return writeBatchImpl(reinterpret_cast<const uint8_t *>(msgs), count, sizeof(T));
  }

  template <typename T>
  [[nodiscard]] size_t readBatch(T *msgs, size_t count) noexcept {
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable for memcpy");
    return readBatchImpl(reinterpret_cast<uint8_t *>(msgs), count, sizeof(T));
  }

  [[nodiscard]] bool write(const uint8_t *data, size_t size) noexcept {
    return writeImpl(data, size);
  }

  [[nodiscard]] bool read(uint8_t *data, size_t size) noexcept { return readImpl(data, size); }

  [[nodiscard]] size_t writeBatch(const uint8_t *data, size_t count, size_t elemSize) noexcept {
    return writeBatchImpl(data, count, elemSize);
  }

  [[nodiscard]] size_t readBatch(uint8_t *data, size_t count, size_t elemSize) noexcept {
    return readBatchImpl(data, count, elemSize);
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
  [[nodiscard]] bool writeImpl(const uint8_t *src, size_t size) noexcept {
    const size_t currentTail = tail_.load(std::memory_order_relaxed);

    if (currentTail + size > headCache_ + BUFFER_SIZE) {
      headCache_ = head_.load(std::memory_order_acquire);
      if (currentTail + size > headCache_ + BUFFER_SIZE) {
        return false;
      }
    }

    const size_t writePos = currentTail & MASK;
    copyIntoRingBytes(writePos, src, size);

    tail_.store(currentTail + size, std::memory_order_release);
    return true;
  }

  [[nodiscard]] bool readImpl(uint8_t *dst, size_t size) noexcept {
    if (readCursor_ + size > tailCache_) {
      head_.store(readCursor_, std::memory_order_release);
      tailCache_ = tail_.load(std::memory_order_acquire);
      if (readCursor_ + size > tailCache_) {
        return false;
      }
    }

    const size_t readPos = readCursor_ & MASK;
    copyFromRingBytes(readPos, dst, size);

    readCursor_ += size;
    return true;
  }

  [[nodiscard]] size_t writeBatchImpl(const uint8_t *src, size_t count, size_t elemSize) noexcept {
    if (count == 0 || elemSize == 0) {
      return 0;
    }
    if (count > MAX_BATCH_SIZE) {
      count = MAX_BATCH_SIZE;
    }

    const size_t currentTail = tail_.load(std::memory_order_relaxed);

    size_t available = headCache_ + BUFFER_SIZE - currentTail;
    if (available < elemSize) {
      headCache_ = head_.load(std::memory_order_acquire);
      available = headCache_ + BUFFER_SIZE - currentTail;
      if (available < elemSize) {
        return 0;
      }
    }

    size_t n = available / elemSize;
    if (n > count) {
      n = count;
    }

    copyIntoRingBytes(currentTail & MASK, src, n * elemSize);

    tail_.store(currentTail + n * elemSize, std::memory_order_release);
    return n;
  }

  [[nodiscard]] size_t readBatchImpl(uint8_t *dst, size_t count, size_t elemSize) noexcept {
    if (count == 0 || elemSize == 0) {
      return 0;
    }
    if (count > MAX_BATCH_SIZE) {
      count = MAX_BATCH_SIZE;
    }

    if (readCursor_ + elemSize > tailCache_) {
      head_.store(readCursor_, std::memory_order_release);
      tailCache_ = tail_.load(std::memory_order_acquire);
      if (readCursor_ + elemSize > tailCache_) {
        return 0;
      }
    }

    const size_t available = tailCache_ - readCursor_;
    size_t n = available / elemSize;
    if (n > count) {
      n = count;
    }

    copyFromRingBytes(readCursor_ & MASK, dst, n * elemSize);

    readCursor_ += n * elemSize;
    return n;
  }

  void copyIntoRingBytes(size_t writePos, const uint8_t *src, size_t bytes) noexcept {
    if (writePos + bytes <= BUFFER_SIZE) {
      std::memcpy(buffer_ + writePos, src, bytes);
    } else {
      const size_t first = BUFFER_SIZE - writePos;
      std::memcpy(buffer_ + writePos, src, first);
      std::memcpy(buffer_, src + first, bytes - first);
    }
  }

  void copyFromRingBytes(size_t readPos, uint8_t *dst, size_t bytes) noexcept {
    if (readPos + bytes <= BUFFER_SIZE) {
      std::memcpy(dst, buffer_ + readPos, bytes);
    } else {
      const size_t first = BUFFER_SIZE - readPos;
      std::memcpy(dst, buffer_ + readPos, first);
      std::memcpy(dst + first, buffer_, bytes - first);
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
