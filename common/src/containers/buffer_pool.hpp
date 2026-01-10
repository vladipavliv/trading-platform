/**
 * @author Vladimir Pavliv
 * @date 2026-01-03
 */

#ifndef HFT_COMMON_BUFFERPOOL_HPP
#define HFT_COMMON_BUFFERPOOL_HPP

#include "primitive_types.hpp"

#include <array>
#include <cassert>
#include <stdexcept>

namespace hft {

struct BufferPtr {
  uint8_t *data{nullptr};
  uint32_t index{0};

  BufferPtr() = default;

  BufferPtr(uint8_t *data, uint32_t index) : data{data}, index{index} {};

  BufferPtr(const BufferPtr &) = delete;
  BufferPtr &operator=(const BufferPtr &) = delete;

  BufferPtr(BufferPtr &&) = default;

  constexpr explicit operator bool() const noexcept { return data != nullptr; }
  constexpr bool operator!() const noexcept { return data == nullptr; }
};

template <uint32_t BufferCapacity = 128, uint32_t PoolSize = 32 * 1024>
class BufferPool {
public:
  static_assert(BufferCapacity % 64 == 0);

  static constexpr uint32_t BUFFER_CAPACITY = BufferCapacity;
  static constexpr uint32_t POOL_SIZE = PoolSize;

  inline static auto instance() -> BufferPool<BufferCapacity, PoolSize> & {
    static BufferPool *instance = new BufferPool<BufferCapacity, PoolSize>();
    return *instance;
  }

  BufferPool() : freePtr_(PoolSize) {
    for (uint32_t i = 0; i < PoolSize; ++i) {
      freeIndices_[i] = i;
    }
  }

  inline auto acquire() -> BufferPtr {
    if (freePtr_ == 0) [[unlikely]] {
      return BufferPtr{};
    }
    uint32_t idx = freeIndices_[--freePtr_];
    return {&storage_[idx * BufferCapacity], idx};
  }

  inline void release(uint32_t index) noexcept {
    assert(index < PoolSize);
    assert(freePtr_ < PoolSize);
    freeIndices_[freePtr_++] = index;
  }

private:
  std::array<uint8_t, BufferCapacity * PoolSize> storage_;
  std::array<uint32_t, PoolSize> freeIndices_;
  uint32_t freePtr_;
};

} // namespace hft

#endif // HFT_COMMON_BUFFERPOOL_HPP
