/**
 * @author Vladimir Pavliv
 * @date 2026-01-10
 */

#ifndef HFT_COMMON_SLOTBUFFER_HPP
#define HFT_COMMON_SLOTBUFFER_HPP

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "constants.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"

namespace hft {

class SlotBuffer {
  static constexpr uint32_t SlotCount = 65536;
  static constexpr uint32_t DataCapacity = 60;

  static_assert((SlotCount & (SlotCount - 1)) == 0);

  struct alignas(64) Slot {
    AtomicUInt32 seq{};
    uint8_t data[DataCapacity];
  };

  static constexpr uint32_t MASK = SlotCount - 1;

public:
  SlotBuffer() {
    for (uint32_t i = 0; i < SlotCount; ++i) {
      slots_[i].seq.store(i, std::memory_order_relaxed);
    }
  }

  inline bool write(const uint8_t *__restrict__ src, uint32_t len) noexcept {
    if (len > DataCapacity) {
      LOG_ERROR_SYSTEM("Message is too long {}", len);
      return false;
    }
    Slot &slot = slots_[pIdx_ & MASK];

    uint32_t spins = 0;
    auto seq = slot.seq.load(std::memory_order_acquire);
    while (seq != pIdx_ && ++spins < BUSY_WAIT_CYCLES) {
      asm volatile("pause" ::: "memory");
      seq = slot.seq.load(std::memory_order_acquire);
    }
    if (seq != pIdx_) {
      return false;
    }
    std::memcpy(slot.data, src, len);

    slot.seq.store(pIdx_ + 1, std::memory_order_release);
    ++pIdx_;
    return true;
  }

  inline uint32_t read(uint8_t *__restrict__ dst, uint32_t maxLen) noexcept {
    Slot &slot = slots_[cIdx_ & MASK];

    uint32_t len = std::min(DataCapacity, maxLen);
    uint32_t spins = 0;
    uint32_t s = slot.seq.load(std::memory_order_acquire);
    while (s != cIdx_ + 1 && ++spins < BUSY_WAIT_CYCLES) {
      asm volatile("pause" ::: "memory");
      s = slot.seq.load(std::memory_order_acquire);
    }
    if (s != cIdx_ + 1) {
      return 0;
    }

    std::memcpy(dst, slot.data, len);

    slot.seq.store(cIdx_ + SlotCount, std::memory_order_release);
    cIdx_++;
    return len;
  }

private:
  alignas(64) Slot slots_[SlotCount];

  alignas(64) uint32_t pIdx_{0};
  alignas(64) uint32_t cIdx_{0};
};

} // namespace hft

#endif // HFT_COMMON_SLOTBUFFER_HPP
