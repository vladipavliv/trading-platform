/**
 * @author Vladimir Pavliv
 * @date 2026-01-03
 */

#ifndef HFT_COMMON_BUFFERPOOL_HPP
#define HFT_COMMON_BUFFERPOOL_HPP

#include "types.hpp"

#include <array>
#include <stdexcept>

namespace hft {

template <size_t BufferSize = 128, size_t PoolSize = 1024 * 1024>
class BufferPool {
public:
  static constexpr size_t BUFFER_SIZE = BufferSize;
  static constexpr size_t POOL_SIZE = PoolSize;

  inline static auto instance() -> BufferPool<BufferSize, PoolSize> & {
    static SPtr<BufferPool<BufferSize, PoolSize>> instance = nullptr;
    if (instance == nullptr) {
      instance = std::make_shared<BufferPool<BufferSize, PoolSize>>();
    }
    return *instance;
  }

  struct Lease {
    uint8_t *data{nullptr};
    size_t size{0};
    size_t index{0};
  };

  BufferPool() : freePtr_(PoolSize) {
    for (size_t i = 0; i < PoolSize; ++i) {
      freeIndices_[i] = i;
    }
  }

  inline auto acquire() -> Lease {
    if (freePtr_ == 0) [[unlikely]] {
      return Lease{};
    }
    size_t idx = freeIndices_[--freePtr_];
    return {&storage_[idx * BufferSize], BufferSize, idx};
  }

  inline void release(size_t index) noexcept { freeIndices_[freePtr_++] = index; }

private:
  alignas(64) std::array<uint8_t, BufferSize * PoolSize> storage_;
  std::array<size_t, PoolSize> freeIndices_;
  size_t freePtr_;
};

} // namespace hft

#endif // HFT_COMMON_BUFFERPOOL_HPP