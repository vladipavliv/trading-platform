/**
 * @author Vladimir Pavliv
 * @date 2026-01-10
 */

#ifndef HFT_COMMON_SLOTHBUFFER_HPP
#define HFT_COMMON_SLOTHBUFFER_HPP

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "constants.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"

namespace hft {

class SlothBuffer {
  static constexpr uint32_t SlotCount = 128 * 1024;
  static constexpr uint32_t DataCapacity = 56;

  static_assert((SlotCount & (SlotCount - 1)) == 0);

  struct alignas(64) Sloth {
    AtomicUInt64 seq{};
    uint32_t length{};
    uint8_t data[DataCapacity] = {};
  };

  static constexpr uint32_t MASK = SlotCount - 1;

public:
  SlothBuffer() {
    for (uint32_t i = 0; i < SlotCount; ++i) {
      slots_[i].seq.store(i, std::memory_order_relaxed);
    }
  }

  inline bool write(const uint8_t *__restrict__ src, uint32_t length) noexcept {
    if (length > DataCapacity) {
      return false;
    }
    Sloth &sloth = slots_[pIdx_ & MASK];

    uint32_t spins = 0;
    auto seq = sloth.seq.load(std::memory_order_acquire);
    while (seq != pIdx_ && ++spins < BUSY_WAIT_CYCLES) {
      asm volatile("pause" ::: "memory");
      seq = sloth.seq.load(std::memory_order_acquire);
    }
    if (seq != pIdx_) {
      LOG_ERROR("Failed to write message to slot buffer {}", pIdx_);
      return false;
    }
    std::memcpy(sloth.data, src, length);

    sloth.length = length;
    sloth.seq.store(pIdx_ + 1, std::memory_order_release);
    ++pIdx_;
    return true;
  }

  inline uint32_t read(uint8_t *__restrict__ dst, uint32_t maxLen) noexcept {
    Sloth &sloth = slots_[cIdx_ & MASK];

    uint32_t spins = 0;
    uint64_t seq = sloth.seq.load(std::memory_order_acquire);
    while (seq != cIdx_ + 1 && ++spins < BUSY_WAIT_CYCLES) {
      asm volatile("pause" ::: "memory");
      seq = sloth.seq.load(std::memory_order_acquire);
    }
    if (seq != cIdx_ + 1) {
      return 0;
    }
    if (sloth.length > maxLen) {
      LOG_ERROR("Buffer is too small, data {} buffer {}", sloth.length, maxLen);
      return false;
    }

    std::memcpy(dst, sloth.data, sloth.length);

    sloth.seq.store(cIdx_ + SlotCount, std::memory_order_release);
    cIdx_++;
    return sloth.length;
  }

private:
  alignas(64) Sloth slots_[SlotCount];

  alignas(64) uint64_t pIdx_{0};
  alignas(64) uint64_t cIdx_{0};
};

} // namespace hft

#endif // HFT_COMMON_SLOTHBUFFER_HPP
