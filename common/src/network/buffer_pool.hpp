/**
 * @author Vladimir Pavliv
 * @date 2026-01-03
 */

#ifndef HFT_COMMON_BUFFERPOOL_HPP
#define HFT_COMMON_BUFFERPOOL_HPP

#include "primitive_types.hpp"

#include <array>
#include <stdexcept>

namespace hft {

struct BufferLease {
  uint8_t *data{nullptr};
  uint32_t size{0};
  uint32_t index{0};
  uint32_t capacity{0};

  constexpr explicit operator bool() const noexcept { return data != nullptr; }
  constexpr bool operator!() const noexcept { return data == nullptr; }
};

template <uint32_t BufferSize = 128, uint32_t PoolSize = 32 * 1024>
class BufferPool {
public:
  static constexpr uint32_t BUFFER_SIZE = BufferSize;
  static constexpr uint32_t POOL_SIZE = PoolSize;

  inline static auto instance() -> BufferPool<BufferSize, PoolSize> & {
    static UPtr<BufferPool<BufferSize, PoolSize>> instance = nullptr;
    if (instance == nullptr) {
      instance = std::make_unique<BufferPool<BufferSize, PoolSize>>();
    }
    return *instance;
  }

  BufferPool() : freePtr_(PoolSize) {
    for (uint32_t i = 0; i < PoolSize; ++i) {
      freeIndices_[i] = i;
    }
  }

  inline auto acquire() -> BufferLease {
    if (freePtr_ == 0) [[unlikely]] {
      return BufferLease{};
    }
    uint32_t idx = freeIndices_[--freePtr_];
    return {&storage_[idx * BufferSize], BufferSize, idx, BufferSize};
  }

  inline void release(uint32_t index) noexcept {
    assert(index < PoolSize && "Invalid BufferPool index");
    freeIndices_[freePtr_++] = index;
  }

private:
  alignas(64) std::array<uint8_t, BufferSize * PoolSize> storage_;
  std::array<uint32_t, PoolSize> freeIndices_;
  uint32_t freePtr_;
};

} // namespace hft

#endif // HFT_COMMON_BUFFERPOOL_HPP
