/**
 * @author Vladimir Pavliv
 * @date 2025-08-08
 */

#ifndef HFT_COMMON_MPSCQUEUE_HPP
#define HFT_COMMON_MPSCQUEUE_HPP

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace hft {

template <typename Value, size_t Capacity>
class MpscQueue {
  static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of two");

public:
  MpscQueue() : head_(0), tail_(0) {}

  bool enqueue(const Value &item) {
    size_t tail = tail_.fetch_add(1, std::memory_order_acq_rel);
    size_t head = head_;

    if (tail - head >= Capacity) {
      return false;
    }

    buffer_[tail & mask_] = item;
    return true;
  }

  bool enqueue(Value &&item) {
    size_t tail = tail_.fetch_add(1, std::memory_order_acq_rel);
    size_t head = head_;

    if (tail - head >= Capacity) {
      return false;
    }

    buffer_[tail & mask_] = std::move(item);
    return true;
  }

  bool dequeue(Value &out) {
    size_t head = head_;
    size_t tail = tail_.load(std::memory_order_acquire);

    if (head == tail) {
      return false;
    }

    out = std::move(buffer_[head & mask_]);
    head_ = head + 1;
    return true;
  }

  bool empty() const { return head_ == tail_.load(std::memory_order_acquire); }

  void clear() {
    head_ = 0;
    tail_ = 0;
  }

  size_t size() const { return tail_.load(std::memory_order_acquire) - head_; }

  size_t capacity() const { return Capacity; }

private:
  static constexpr size_t mask_ = Capacity - 1;

  alignas(64) std::atomic<size_t> tail_;
  alignas(64) size_t head_;
  alignas(64) Value buffer_[Capacity];
};

} // namespace hft

#endif // HFT_COMMON_MPSCQUEUE_HPP