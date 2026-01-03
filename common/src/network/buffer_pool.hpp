/**
 * @author Vladimir Pavliv
 * @date 2025-08-12
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

  // The Lease is now just a plain-old-data (POD) view
  struct Lease {
    uint8_t *data;
    size_t size;
    size_t index;
  };

  BufferPool() : free_ptr_(PoolSize) {
    for (size_t i = 0; i < PoolSize; ++i) {
      free_indices_[i] = i;
    }
  }

  inline auto acquire() -> Lease {
    if (free_ptr_ == 0) [[unlikely]] {
      throw std::runtime_error("BufferPool exhausted");
    }
    size_t idx = free_indices_[--free_ptr_];
    return {&storage_[idx * BufferSize], BufferSize, idx};
  }

  // Manual release: caller must ensure index is valid
  inline void release(size_t index) noexcept { free_indices_[free_ptr_++] = index; }

private:
  alignas(64) std::array<uint8_t, BufferSize * PoolSize> storage_;
  std::array<size_t, PoolSize> free_indices_;
  size_t free_ptr_;
};

} // namespace hft

#endif // HFT_COMMON_BUFFERPOOL_HPP