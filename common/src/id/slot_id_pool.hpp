/**
 * @author Vladimir Pavliv
 * @date 2026-01-14
 */

#ifndef HFT_SERVER_SLOTIDPOOL_HPP
#define HFT_SERVER_SLOTIDPOOL_HPP

#include "constants.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"
#include "slot_id.hpp"
#include "utils/huge_array.hpp"

namespace hft {

/**
 * @brief spsc thread-safe pool of indexes
 */
template <uint32_t MaxCapacity = MAX_SYSTEM_ORDERS>
class SlotIdPool {
public:
  using IdType = SlotId<MaxCapacity>;

  static constexpr uint32_t MASK = IdType::maxIndex();
  static constexpr uint32_t CAPACITY = MASK + 1;

  static constexpr uint32_t LOCAL_CACHE_SIZE = 65536;
  static constexpr uint32_t FRESH_CHUNK_SIZE = 16384;

  SlotIdPool() : localTop_(0), nextFreshIdx_(1) {}

  [[nodiscard]] inline IdType acquire() noexcept {
    if (LIKELY(localTop_ > 0)) {
      return localStack_[--localTop_];
    }
    return refill();
  }

  inline void release(IdType id) noexcept {
    LOG_DEBUG("release {}", id.index());
    id.nextGen();
    uint32_t t = tail_.load(std::memory_order_relaxed);

    sharedQueue_[t] = id;
    tail_.store((t + 1) & MASK, std::memory_order_release);
  }

private:
  IdType refill() noexcept {
    LOG_DEBUG("refilling");
    uint32_t h = head_.load(std::memory_order_relaxed);
    uint32_t t = tail_.load(std::memory_order_acquire);

    while (h != t && localTop_ < LOCAL_CACHE_SIZE) {
      localStack_[localTop_++] = sharedQueue_[h];
      h = (h + 1) & MASK;
    }
    head_.store(h, std::memory_order_release);

    if (localTop_ == 0) {
      const uint32_t limit = std::min(nextFreshIdx_ + FRESH_CHUNK_SIZE, CAPACITY);

      while (nextFreshIdx_ < limit && localTop_ < LOCAL_CACHE_SIZE) {
        localStack_[localTop_++] = IdType::make(nextFreshIdx_++, 1);
      }
      LOG_DEBUG("Generated fresh chunk of {} IDs", localTop_);
    }

    return (localTop_ > 0) ? localStack_[--localTop_] : IdType{};
  }

private:
  ALIGN_CL AtomicUInt32 head_{0};
  ALIGN_CL AtomicUInt32 tail_{0};

  HugeArray<IdType, CAPACITY> sharedQueue_;

  IdType localStack_[LOCAL_CACHE_SIZE];
  uint32_t localTop_ = 0;
  uint32_t nextFreshIdx_ = 1;
};

} // namespace hft

#endif // HFT_SERVER_SLOTIDPOOL_HPP
