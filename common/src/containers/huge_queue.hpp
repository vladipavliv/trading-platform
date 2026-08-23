/**
 * @author kimi.ai
 * @date 2026-08-23
 */

#ifndef HFT_COMMON_HUGEQUEUE_HPP
#define HFT_COMMON_HUGEQUEUE_HPP

#include "containers/huge_array.hpp"
#include "primitive_types.hpp"

namespace hft {

/**
 * @brief Fixed-capacity ring queue backed by HugeArray
 */
template <typename T, size_t Capacity>
class HugeQueue {
  static_assert(std::is_trivially_copyable_v<T>);

public:
  static constexpr size_t Cap = Capacity;

  HugeQueue() = default;

  HugeQueue(HugeQueue &&other) noexcept
      : buf_(std::move(other.buf_)), head_(other.head_), tail_(other.tail_), size_(other.size_) {
    other.head_ = 0;
    other.tail_ = 0;
    other.size_ = 0;
  }

  HugeQueue &operator=(HugeQueue &&other) noexcept {
    if (this != &other) {
      buf_ = std::move(other.buf_);
      head_ = other.head_;
      tail_ = other.tail_;
      size_ = other.size_;
      other.head_ = 0;
      other.tail_ = 0;
      other.size_ = 0;
    }
    return *this;
  }

  [[nodiscard]] inline bool empty() const noexcept { return size_ == 0; }
  [[nodiscard]] inline size_t size() const noexcept { return size_; }
  [[nodiscard]] static constexpr size_t capacity() noexcept { return Cap; }

  inline void push(const T &v) noexcept {
    assert(size_ < Cap && "HugeQueue overflow");
    buf_[tail_] = v;
    tail_ = (tail_ + 1) % Cap;
    ++size_;
  }

  inline void pop() noexcept {
    assert(size_ > 0 && "HugeQueue underflow");
    head_ = (head_ + 1) % Cap;
    --size_;
  }

  [[nodiscard]] inline T &front() noexcept {
    assert(size_ > 0);
    return buf_[head_];
  }

  [[nodiscard]] inline const T &front() const noexcept {
    assert(size_ > 0);
    return buf_[head_];
  }

  [[nodiscard]] inline T &operator[](size_t i) noexcept {
    assert(i < size_);
    return buf_[(head_ + i) % Cap];
  }

  [[nodiscard]] inline const T &operator[](size_t i) const noexcept {
    assert(i < size_);
    return buf_[(head_ + i) % Cap];
  }

  inline void clear() noexcept {
    head_ = 0;
    tail_ = 0;
    size_ = 0;
    buf_.clear();
  }

  HugeQueue(const HugeQueue &) = delete;
  HugeQueue &operator=(const HugeQueue &) = delete;

private:
  HugeArray<T, Cap> buf_;
  size_t head_{0};
  size_t tail_{0};
  size_t size_{0};
};

} // namespace hft

#endif // HFT_COMMON_HUGEQUEUE_HPP