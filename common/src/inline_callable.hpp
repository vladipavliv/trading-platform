/**
 * @author Vladimir Pavliv
 * @date 2025-08-11
 */

#ifndef HFT_COMMON_INLINECALLABLE_HPP
#define HFT_COMMON_INLINECALLABLE_HPP

#include <cassert>
#include <concepts>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace hft {

/**
 * @brief Inline stack callable with no type erasure
 * @details With simple lambda and small captures benchmarks 20% better then std::function
 * On the real load the difference would likely be even higher
 */
template <typename Arg, std::size_t BufferSize = 128>
class InlineCallable {
public:
  using ArgType = const Arg &;
  using Invoker = void (*)(void *, ArgType);
  using Destroyer = void (*)(void *);
  using Mover = void (*)(void *dest, void *src);
  using Copier = void (*)(void *dest, const void *src);

  InlineCallable() noexcept = default;

  ~InlineCallable() noexcept { reset(); }

  template <typename Lambda>
    requires std::invocable<Lambda &, ArgType> && (sizeof(std::decay_t<Lambda>) <= BufferSize) &&
             (alignof(std::decay_t<Lambda>) <= alignof(std::max_align_t))
  InlineCallable(Lambda &&lambda) noexcept {
    using LambdaType = std::decay_t<Lambda>;

    void *place = &storage_;
    new (place) LambdaType(std::forward<Lambda>(lambda));

    invoker_ = [](void *p, ArgType a) { (*static_cast<LambdaType *>(p))(a); };
    destroyer_ = [](void *p) { static_cast<LambdaType *>(p)->~LambdaType(); };
    mover_ = [](void *dest, void *src) {
      LambdaType *l = static_cast<LambdaType *>(src);
      void *dplace = dest;
      new (dplace) LambdaType(std::move(*l));
      l->~LambdaType();
    };
    copier_ = [](void *dest, const void *src) {
      const LambdaType *l = static_cast<const LambdaType *>(src);
      void *dplace = dest;
      new (dplace) LambdaType(*l);
    };
  }

  InlineCallable(InlineCallable &&other) noexcept {
    invoker_ = other.invoker_;
    destroyer_ = other.destroyer_;
    mover_ = other.mover_;
    copier_ = other.copier_;
    if (invoker_) {
      mover_(&storage_, &other.storage_);
      other.invoker_ = nullptr;
      other.destroyer_ = nullptr;
      other.mover_ = nullptr;
      other.copier_ = nullptr;
    }
  }

  InlineCallable &operator=(InlineCallable &&other) noexcept {
    if (this != &other) {
      reset();
      invoker_ = other.invoker_;
      destroyer_ = other.destroyer_;
      mover_ = other.mover_;
      copier_ = other.copier_;
      if (invoker_) {
        mover_(&storage_, &other.storage_);
        other.invoker_ = nullptr;
        other.destroyer_ = nullptr;
        other.mover_ = nullptr;
        other.copier_ = nullptr;
      }
    }
    return *this;
  }

  InlineCallable(const InlineCallable &other) noexcept {
    invoker_ = other.invoker_;
    destroyer_ = other.destroyer_;
    mover_ = other.mover_;
    copier_ = other.copier_;
    if (invoker_) {
      copier_(&storage_, &other.storage_);
    }
  }

  InlineCallable &operator=(const InlineCallable &other) noexcept {
    if (this != &other) {
      reset();
      invoker_ = other.invoker_;
      destroyer_ = other.destroyer_;
      mover_ = other.mover_;
      copier_ = other.copier_;
      if (invoker_) {
        copier_(&storage_, &other.storage_);
      }
    }
    return *this;
  }

  inline void operator()(ArgType arg) {
    assert(invoker_ && "Empty InlineCallable invoked");
    invoker_(&storage_, arg);
  }

  explicit operator bool() const noexcept { return invoker_ != nullptr; }

  void reset() noexcept {
    if (destroyer_) {
      destroyer_(&storage_);
      destroyer_ = nullptr;
      invoker_ = nullptr;
      mover_ = nullptr;
      copier_ = nullptr;
    }
  }

private:
  alignas(std::max_align_t) unsigned char storage_[BufferSize];

  Invoker invoker_{nullptr};
  Destroyer destroyer_{nullptr};
  Mover mover_{nullptr};
  Copier copier_{nullptr};
};

} // namespace hft

#endif // HFT_COMMON_INLINECALLABLE_HPP