/**
 * @author Vladimir Pavliv
 * @date 2026-01-24
 */

#ifndef HFT_COMMON_BATCHSPSC_HPP
#define HFT_COMMON_BATCHSPSC_HPP

#include "primitive_types.hpp"

namespace hft {

template <typename T, size_t Capacity>
class BatchSPSC {
  static_assert(std::is_trivially_copyable_v<T>);
  static_assert((Capacity & (Capacity - 1)) == 0);

public:
  static constexpr size_t MSG_SIZE = sizeof(T);
  static constexpr size_t BUFFER_SIZE = Capacity * MSG_SIZE;

  BatchSPSC() = default;

  bool write(const T &msg) noexcept {
    const size_t currentTail = tail_.load(std::memory_order_relaxed);

    if (currentTail + MSG_SIZE > headCache_ + BUFFER_SIZE) {
      headCache_ = head_.load(std::memory_order_acquire);
      if (currentTail + MSG_SIZE > headCache_ + BUFFER_SIZE)
        return false;
    }

    const size_t writePos = currentTail & (BUFFER_SIZE - 1);
    std::memcpy(buffer_ + writePos, &msg, MSG_SIZE);

    tail_.store(currentTail + MSG_SIZE, std::memory_order_release);
    return true;
  }

  std::span<T> readBatch(size_t maxBatch = 64) noexcept {
    const size_t currentHead = head_.load(std::memory_order_relaxed);
    const size_t currentTail = tail_.load(std::memory_order_acquire);

    if (currentHead == currentTail)
      return {};

    size_t headPos = currentHead & (BUFFER_SIZE - 1);

    size_t bytesToEnd = BUFFER_SIZE - headPos;
    size_t availableBytes = std::min(currentTail - currentHead, bytesToEnd);

    size_t count = std::min(availableBytes / MSG_SIZE, maxBatch);
    return {reinterpret_cast<T *>(buffer_ + headPos), count};
  }

  void commitRead(size_t count) noexcept {
    if (count == 0) {
      return;
    }
    size_t currentHead = head_.load(std::memory_order_relaxed);
    head_.store(currentHead + (count * MSG_SIZE), std::memory_order_release);
  }

private:
  ALIGN_CL AtomicSizeT head_{0};
  size_t headCache_{0};

  ALIGN_CL AtomicSizeT tail_{0};
  size_t tailCache_{0};

  ALIGN_CL uint8_t buffer_[BUFFER_SIZE];
};

} // namespace hft

#endif // HFT_COMMON_BATCHSPSC_HPP