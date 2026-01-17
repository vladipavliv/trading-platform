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
#include "vyukov_mpmc.hpp"

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
    uint32_t index = 0;
    if (!indexQueue_.pop(index)) {
      index = nextFreeIdx_.fetch_add(1, std::memory_order_relaxed);
      if (index >= PoolSize) {
        LOG_ERROR("BufferPool exhausted");
        return BufferPtr{};
      }
    }
    return {&storage_[index * BufferCapacity], index};
  }

  inline void release(uint32_t index) noexcept {
    if (!indexQueue_.push(index)) {
      LOG_ERROR_SYSTEM("Index double release in BufferPool");
    }
  }

private:
  BufferPool() : nextFreeIdx_(0) {}

  ALIGN_CL std::array<uint8_t, BufferCapacity * PoolSize> storage_;
  ALIGN_CL VyukovMPMC<uint32_t, PoolSize> indexQueue_;
  ALIGN_CL AtomicUInt32 nextFreeIdx_;
};

} // namespace hft

#endif // HFT_COMMON_BUFFERPOOL_HPP