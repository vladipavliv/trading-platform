/**
 * @author Vladimir Pavliv
 * @date 2026-01-10
 */

#ifndef HFT_COMMON_SLOTHSPSCQUEUE_HPP
#define HFT_COMMON_SLOTHSPSCQUEUE_HPP

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "constants.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"
#include "ptr_types.hpp"

namespace hft {

/**
 * @brief Slot based spsc queue for small messages
 */
template <size_t SlotCount = 65536>
class SequencedSPSC {
  static_assert((SlotCount & (SlotCount - 1)) == 0);

  static constexpr uint32_t MAX_DATA_SIZE = 52;
  static constexpr uint32_t MASK = SlotCount - 1;

  struct alignas(64) Sloth {
    AtomicUInt64 seq{};
    uint32_t size{};
    uint8_t data[MAX_DATA_SIZE] = {};
  };

public:
  SequencedSPSC() {
    for (uint32_t i = 0; i < SlotCount; ++i) {
      slots_[i].seq.store(i, std::memory_order_relaxed);
    }
  }

  template <typename T>
  inline bool write(CRef<T> msg) {
    return write(reinterpret_cast<const uint8_t *>(&msg), sizeof(T));
  }

  inline bool write(const uint8_t *__restrict__ src, uint32_t size) noexcept {
    if (size > MAX_DATA_SIZE) {
      return false;
    }
    Sloth &sloth = slots_[writeIdx_ & MASK];
    if (sloth.seq.load(std::memory_order_acquire) != writeIdx_) {
      return false;
    }
    std::memcpy(sloth.data, src, size);

    sloth.size = size;
    sloth.seq.store(writeIdx_ + 1, std::memory_order_release);
    ++writeIdx_;
    return true;
  }

  template <typename T>
  inline uint32_t read(T &msg) {
    return read(reinterpret_cast<uint8_t *>(&msg), sizeof(T));
  }

  inline uint32_t read(uint8_t *__restrict__ dst, uint32_t maxSize) noexcept {
    Sloth &sloth = slots_[readIdx_ & MASK];

    if (sloth.seq.load(std::memory_order_acquire) != readIdx_ + 1) {
      return 0;
    }
    if (sloth.size > maxSize) {
      LOG_ERROR("Buffer is too small, data {} buffer {}", sloth.size, maxSize);
      return false;
    }

    std::memcpy(dst, sloth.data, sloth.size);

    sloth.seq.store(readIdx_ + SlotCount, std::memory_order_release);
    readIdx_++;
    return sloth.size;
  }

private:
  // data first so it fills up the huge pages nicely
  alignas(64) Sloth slots_[SlotCount];

  // control block is separated to the next huge page
  alignas(64) uint64_t writeIdx_{0};
  alignas(64) uint64_t readIdx_{0};
};

} // namespace hft

#endif // HFT_COMMON_SLOTHSPSCQUEUE_HPP
