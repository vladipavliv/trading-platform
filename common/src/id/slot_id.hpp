/**
 * @author Vladimir Pavliv
 * @date 2026-01-14
 */

#ifndef HFT_SERVER_SLOTID_HPP
#define HFT_SERVER_SLOTID_HPP

#include "primitive_types.hpp"
#include "utils/binary_utils.hpp"

namespace hft {
/**
 * @brief Versioned handle for O(1) array-access with ABA protection
 * Combines a static index with generation counter to allow safe reuse in flat arrays
 */
template <uint32_t MaxCapacity = 16777216>
class SlotId {
public:
  static constexpr uint8_t IndexBits = utils::bitWidth(MaxCapacity);
  static constexpr uint8_t GenBits = 32 - IndexBits;

  static_assert(IndexBits < 32, "Capacity too large for uint32 storage");
  static_assert(GenBits >= 1, "No bits left for generation counter");

  static constexpr uint32_t INDEX_MASK = (1U << IndexBits) - 1;
  static constexpr uint32_t GEN_MASK = (1U << GenBits) - 1;

  SlotId() noexcept = default;
  explicit constexpr SlotId(uint32_t raw) : value_(raw) {}

  static constexpr SlotId make(uint32_t index, uint32_t gen) {
    assert(index <= INDEX_MASK);
    assert(gen <= GEN_MASK);
    return SlotId(((gen & GEN_MASK) << IndexBits) | (index & INDEX_MASK));
  }

  [[nodiscard]] constexpr uint32_t index() const noexcept {
    return static_cast<uint32_t>(value_ & INDEX_MASK);
  }

  [[nodiscard]] constexpr uint32_t gen() const noexcept {
    return static_cast<uint32_t>((value_ >> IndexBits) & GEN_MASK);
  }

  [[nodiscard]] constexpr uint32_t raw() const noexcept { return value_; }
  [[nodiscard]] constexpr bool isValid() const noexcept { return gen() != 0; }

  inline void nextGen() noexcept {
    uint32_t g = (gen() + 1) & GEN_MASK;

    if (UNLIKELY(g == 0)) {
      g = 1;
    }

    value_ = (value_ & INDEX_MASK) | (g << IndexBits);
  }

  constexpr bool operator==(const SlotId &other) const noexcept { return value_ == other.value_; }
  constexpr bool operator!=(const SlotId &other) const noexcept { return value_ != other.value_; }
  explicit constexpr operator bool() const noexcept { return isValid(); }

  static constexpr uint32_t capacity() { return MaxCapacity; }
  static constexpr uint32_t indexBits() { return IndexBits; }
  static constexpr uint32_t genBits() { return GenBits; }
  static constexpr uint32_t maxIndex() { return INDEX_MASK; }
  static constexpr uint32_t maxGen() { return GEN_MASK; }

private:
  uint32_t value_;
};
} // namespace hft

#endif // HFT_SERVER_SLOTID_HPP
