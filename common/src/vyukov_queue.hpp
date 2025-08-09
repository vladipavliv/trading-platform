/**
 * @author Vladimir Pavliv
 * @date 2025-08-08
 */

#ifndef HFT_COMMON_VYUKOVQUEUE_HPP
#define HFT_COMMON_VYUKOVQUEUE_HPP

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace hft {

/**
 * @brief Dmitry Vyukov's mpmc queue
 * @details https://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
 */
template <typename Value, size_t Capacity>
class VyukovQueue {
  static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");
  static_assert(Capacity <= (1ULL << 20), "Capacity is too large");

  struct alignas(64) Slot {
    std::atomic<size_t> sequence;
    Value value;
  };

public:
  VyukovQueue() {
    for (size_t i = 0; i < Capacity; ++i) {
      slots_[i].sequence.store(i, std::memory_order_relaxed);
    }
    head_.store(0, std::memory_order_relaxed);
    tail_.store(0, std::memory_order_relaxed);
  }

  bool push(const Value &item) {
    size_t tail = tail_.load(std::memory_order_relaxed);
    Slot *slot;

    while (true) {
      slot = &slots_[tail & mask_];
      size_t seq = slot->sequence.load(std::memory_order_acquire);
      intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(tail);

      if (diff == 0) {
        if (tail_.compare_exchange_weak(tail, tail + 1, std::memory_order_relaxed)) {
          break;
        }
      } else if (diff < 0) {
        return false;
      } else {
        tail = tail_.load(std::memory_order_relaxed);
      }
    }

    slot->value = item;
    slot->sequence.store(tail + 1, std::memory_order_release);
    return true;
  }

  bool push(Value &&item) {
    size_t tail = tail_.load(std::memory_order_relaxed);
    Slot *slot;

    while (true) {
      slot = &slots_[tail & mask_];
      size_t seq = slot->sequence.load(std::memory_order_acquire);
      intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(tail);

      if (diff == 0) {
        if (tail_.compare_exchange_weak(tail, tail + 1, std::memory_order_relaxed)) {
          break;
        }
      } else if (diff < 0) {
        return false;
      } else {
        tail = tail_.load(std::memory_order_relaxed);
      }
    }

    slot->value = std::move(item);
    slot->sequence.store(tail + 1, std::memory_order_release);
    return true;
  }

  bool pop(Value &out) {
    size_t head = head_.load(std::memory_order_relaxed);
    Slot *slot;

    while (true) {
      slot = &slots_[head & mask_];
      size_t seq = slot->sequence.load(std::memory_order_acquire);
      intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(head + 1);

      if (diff == 0) {
        if (head_.compare_exchange_weak(head, head + 1, std::memory_order_relaxed)) {
          break;
        }
      } else if (diff < 0) {
        return false;
      } else {
        head = head_.load(std::memory_order_relaxed);
      }
    }

    out = std::move(slot->value);
    slot->sequence.store(head + Capacity, std::memory_order_release);
    return true;
  }

  bool empty() const {
    size_t head = head_.load(std::memory_order_acquire);
    size_t tail = tail_.load(std::memory_order_acquire);
    return head >= tail;
  }

  size_t size() const {
    size_t tail = tail_.load(std::memory_order_acquire);
    size_t head = head_.load(std::memory_order_acquire);
    return tail - head;
  }

private:
  static constexpr size_t mask_ = Capacity - 1;

  alignas(64) std::atomic<size_t> head_;
  alignas(64) std::atomic<size_t> tail_;
  alignas(64) Slot slots_[Capacity];
};

} // namespace hft

#endif // HFT_COMMON_VYUKOVQUEUE_HPP