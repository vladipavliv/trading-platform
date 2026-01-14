/**
 * @author Vladimir Pavliv
 * @date 2026-01-14
 */

#ifndef HFT_SERVER_SLOTID_HPP
#define HFT_SERVER_SLOTID_HPP

#include "primitive_types.hpp"

namespace hft {
/**
 * @brief Versioned handle for O(1) array-access with ABA protection
 * Combines a static index with generation counter to allow safe reuse in flat arrays
 */
template <typename ValueType = uint32_t, uint8_t IndexBits = 24, uint8_t GenBits = 8>
class SlotId {
  static_assert(IndexBits + GenBits <= sizeof(ValueType) * 8, "Bit count exceeds storage type");

  static constexpr ValueType INDEX_MASK = (ValueType{1} << IndexBits) - 1;
  static constexpr ValueType GEN_MASK = (ValueType{1} << GenBits) - 1;

public:
  using StorageType = ValueType;

  constexpr SlotId() : value_(0) {}
  explicit constexpr SlotId(StorageType raw) : value_(raw) {}

  static constexpr SlotId make(uint32_t index, uint32_t gen) {
    assert(index <= INDEX_MASK);
    assert(gen <= GEN_MASK && gen != 0);
    return SlotId((static_cast<StorageType>(gen) << IndexBits) | (index & INDEX_MASK));
  }

  [[nodiscard]] constexpr uint32_t index() const noexcept {
    return static_cast<uint32_t>(value_ & INDEX_MASK);
  }

  [[nodiscard]] constexpr uint32_t gen() const noexcept {
    return static_cast<uint32_t>((value_ >> IndexBits) & GEN_MASK);
  }

  [[nodiscard]] constexpr StorageType raw() const noexcept { return value_; }
  [[nodiscard]] constexpr bool isValid() const noexcept { return value_ != 0; }

  inline void nextGen() noexcept {
    StorageType g = (gen() + 1) & GEN_MASK;

    if (UNLIKELY(g == 0)) {
      g = 1;
    }

    value_ = (value_ & INDEX_MASK) | (g << IndexBits);
  }

  constexpr bool operator==(const SlotId &other) const noexcept { return value_ == other.value_; }
  constexpr bool operator!=(const SlotId &other) const noexcept { return value_ != other.value_; }

  static constexpr uint32_t maxIndex() { return INDEX_MASK; }
  static constexpr uint32_t maxGen() { return GEN_MASK; }

private:
  StorageType value_;
};

} // namespace hft

#endif // HFT_SERVER_SLOTID_HPP