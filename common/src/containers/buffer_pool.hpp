/**
 * @author Vladimir Pavliv
 * @date 2026-01-05
 */

#ifndef HFT_COMMON_BUFFERPOOL_HPP
#define HFT_COMMON_BUFFERPOOL_HPP

#include <array>
#include <atomic>
#include <cassert>
#include <memory>

#include "logging.hpp"
#include "primitive_types.hpp"

namespace hft {

/**
 * @brief
 */
struct BufferPtr {
  uint8_t *data{nullptr};
  uint32_t index{0};

  BufferPtr() = default;
  BufferPtr(uint8_t *data, uint32_t index) : data{data}, index{index} {}

  BufferPtr(const BufferPtr &) = delete;
  BufferPtr &operator=(const BufferPtr &) = delete;
  BufferPtr(BufferPtr &&) = default;
  BufferPtr &operator=(BufferPtr &&) = default;

  constexpr explicit operator bool() const noexcept { return data != nullptr; }
};

/**
 * @brief
 */
template <uint32_t BufferCapacity = 128, uint32_t PoolSize = 1024>
class BufferPool {
public:
  static_assert(BufferCapacity % 64 == 0, "BufferCapacity should be cache-line aligned");

  static constexpr uint32_t BUFFER_CAPACITY = BufferCapacity;
  static constexpr uint32_t POOL_SIZE = PoolSize;

  inline static auto instance() -> BufferPool<BufferCapacity, PoolSize> & {
    static std::unique_ptr<BufferPool> instance(new BufferPool());
    return *instance;
  }

  BufferPool(const BufferPool &) = delete;
  BufferPool &operator=(const BufferPool &) = delete;

  inline auto acquire() -> BufferPtr {
    uint32_t current = freePtr_.load(std::memory_order_relaxed);
    while (true) {
      if (current == 0) [[unlikely]] {
        LOG_ERROR("BufferPool exhausted");
        return BufferPtr{};
      }

      uint32_t next = current - 1;
      if (freePtr_.compare_exchange_weak(current, next, std::memory_order_acquire,
                                         std::memory_order_relaxed)) {
        uint32_t idx = freeIndices_[next];
        return {&storage_[idx * BufferCapacity], idx};
      }
    }
  }

  inline void release(uint32_t index) noexcept {
    uint32_t current = freePtr_.load(std::memory_order_relaxed);
    while (true) {
      if (current >= PoolSize) [[unlikely]] {
        LOG_ERROR("Double release detected for index {}", index);
        assert(false);
        return;
      }

      uint32_t next = current + 1;
      freeIndices_[current] = index;
      if (freePtr_.compare_exchange_weak(current, next, std::memory_order_release,
                                         std::memory_order_relaxed)) {
        return;
      }
    }
  }

private:
  BufferPool() : freePtr_(PoolSize) {
    for (uint32_t i = 0; i < PoolSize; ++i) {
      freeIndices_[i] = i;
    }
    std::memset(storage_.data(), 0, BufferCapacity * PoolSize);
  }

  alignas(64) std::array<uint8_t, BufferCapacity * PoolSize> storage_;
  alignas(64) std::array<uint32_t, PoolSize> freeIndices_;
  alignas(64) std::atomic<uint32_t> freePtr_;
};

} // namespace hft

#endif // HFT_COMMON_BUFFERPOOL_HPP