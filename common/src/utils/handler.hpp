/**
 * @author Vladimir Pavliv
 * @date 2026-01-22
 */

#ifndef HFT_COMMON_HANDLER_HPP
#define HFT_COMMON_HANDLER_HPP

#include <utility>

namespace hft {

template <typename Signature>
struct Handler;

template <typename... Args>
struct Handler<void(Args...)> {
  using FnPtr = void (*)(void *, Args...);

  void *ctx{nullptr};
  FnPtr clb{nullptr};

  template <typename T, void (T::*func)(Args...)>
  static void bridge(void *ctx, Args... args) {
    (static_cast<T *>(ctx)->*func)(std::forward<Args>(args)...);
  }

  template <typename T, void (T::*func)(Args...)>
  static constexpr Handler bind(T *ctx) {
    return {ctx, &bridge<T, func>};
  }

  inline void operator()(Args... args) const {
    if (clb) {
      clb(ctx, std::forward<Args>(args)...);
    }
  }

  explicit operator bool() const { return ctx != nullptr && clb != nullptr; }
};

template <typename Event>
using CRefHandler = Handler<void(const Event &)>;

template <typename Event>
using MoveHandler = Handler<void(Event &&)>;

using Callback = Handler<void()>;

} // namespace hft

#endif // HFT_COMMON_HANDLER_HPP