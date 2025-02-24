/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_BUFFERPOOL_HPP
#define HFT_COMMON_BUFFERPOOL_HPP

#include <atomic>
#include <thread>
#include <vector>

#include "types.hpp"

namespace hft {

struct BufferPool {
  BufferPool(size_t size) : data(size) {}

  uint8_t *acquire(size_t size) {
    auto currHead = head.load(std::memory_order_acquire);
    auto newHead = currHead + size > data.size() ? 0 : currHead + size;

    auto currTail = tail.load(std::memory_order_acquire);
    while ((newHead > currHead) ? (currHead < currTail && currTail < newHead)
                                : (currHead < currTail || currTail < newHead)) {
      currTail = tail.load(std::memory_order_acquire);
      std::this_thread::yield();
    }

    auto expectedHead = currHead;
    while (!head.compare_exchange_weak(currHead, newHead, std::memory_order_release,
                                       std::memory_order_acquire)) {
      currHead = expectedHead;
      newHead = currHead + size > data.size() ? 0 : currHead + size;
    }
    return &data[newHead];
  }

  void release(size_t size) {
    auto currentTail = tail.load(std::memory_order_acquire);
    auto nextTail = currentTail + size > data.size() ? 0 : currentTail + size;

    auto expectedTail = currentTail;
    while (!tail.compare_exchange_weak(currentTail, nextTail, std::memory_order_release,
                                       std::memory_order_acquire)) {
      currentTail = expectedTail;
      nextTail = currentTail + size > data.size() ? 0 : currentTail + size;
    }
  }

  std::atomic_size_t head;
  std::atomic_size_t tail;
  std::vector<uint8_t> data;
};

struct BufferGuard {
  BufferGuard(BufferPool &bPool, size_t bSize) : size{bSize}, pool{bPool} {}
  ~BufferGuard() { pool.release(size); }

  const size_t size;
  BufferPool &pool;
};

} // namespace hft

#endif // HFT_COMMON_BUFFERPOOL_HPP
