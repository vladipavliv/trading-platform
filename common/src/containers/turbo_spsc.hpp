/**
 * @author Vladimir Pavliv
 * @date 2026-01-24
 */

#ifndef HFT_COMMON_TURBOSPSC_HPP
#define HFT_COMMON_TURBOSPSC_HPP

#include "primitive_types.hpp"

namespace hft {

template <size_t Capacity = 65536>
class TurboSPSC {
  static_assert((Capacity & (Capacity - 1)) == 0);

public:
  static constexpr size_t BUFFER_SIZE = Capacity * 64;
  static constexpr size_t MAX_BATCH_SIZE = 64;

  TurboSPSC() = default;

  template <typename T>
  [[nodiscard]] bool write(const T &msg) noexcept {
    const size_t currentTail = tail_.load(std::memory_order_relaxed);
    const size_t size = sizeof(T);

    if (currentTail + size > headCache_ + BUFFER_SIZE) {
      headCache_ = head_.load(std::memory_order_acquire);
      if (currentTail + size > headCache_ + BUFFER_SIZE)
        return false;
    }

    const size_t writePos = currentTail & (BUFFER_SIZE - 1);
    std::memcpy(buffer_ + writePos, &msg, size);

    tail_.store(currentTail + size, std::memory_order_release);
    return true;
  }

  template <typename T>
  [[nodiscard]] bool read(T &msg) noexcept {
    const size_t size = sizeof(T);

    if (readCursor_ == tailCache_) {
      head_.store(readCursor_, std::memory_order_release);

      tailCache_ = tail_.load(std::memory_order_acquire);

      if (readCursor_ == tailCache_) {
        return false;
      }
    }

    const size_t readPos = readCursor_ & (BUFFER_SIZE - 1);
    std::memcpy(&msg, buffer_ + readPos, size);

    readCursor_ += size;
    return true;
  }

private:
  ALIGN_CL AtomicSizeT head_{0};
  size_t tailCache_{0};
  size_t readCursor_{0};

  ALIGN_CL AtomicSizeT tail_{0};
  size_t headCache_{0};

  ALIGN_CL uint8_t buffer_[BUFFER_SIZE];
};

} // namespace hft

#endif // HFT_COMMON_TURBOSPSC_HPP