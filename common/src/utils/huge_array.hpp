/**
 * @author Vladimir Pavliv
 * @date 2026-01-14
 */

#ifndef HFT_COMMON_HUGEARRAY_HPP
#define HFT_COMMON_HUGEARRAY_HPP

#include "memory_utils.hpp"
#include "primitive_types.hpp"

namespace hft {

/**
 * @brief Static array allocated in huge pages
 */
template <typename T, size_t Capacity>
class HugeArray {
  static_assert(std::is_trivially_constructible_v<T>);
  static_assert(std::is_trivially_destructible_v<T>);

public:
  static constexpr size_t ByteSize = Capacity * sizeof(T);

  explicit HugeArray() { data_ = utils::allocHuge<T>(ByteSize); }

  HugeArray(HugeArray &&other) noexcept : data_(other.data_) { other.data_ = nullptr; }

  HugeArray &operator=(HugeArray &&other) noexcept {
    if (this != &other) {
      if (data_) {
        utils::freeHuge(data_, ByteSize);
      }
      data_ = other.data_;
      other.data_ = nullptr;
    }
    return *this;
  }

  ~HugeArray() { utils::freeHuge(data_, ByteSize); }

  [[nodiscard]] inline T &operator[](size_t index) noexcept { return data_[index]; }

  [[nodiscard]] inline const T &operator[](size_t index) const noexcept { return data_[index]; }

  static constexpr size_t capacity() noexcept { return Capacity; }
  static constexpr size_t size_bytes() noexcept { return ByteSize; }

  HugeArray(const HugeArray &) = delete;
  HugeArray &operator=(const HugeArray &) = delete;

private:
  T *data_ = nullptr;
};

} // namespace hft

#endif // HFT_COMMON_HUGEARRAY_HPP