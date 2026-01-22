/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_FUNCTIONALTYPES_HPP
#define HFT_COMMON_FUNCTIONALTYPES_HPP

#include <expected>
#include <functional>
#include <optional>

#include "status_code.hpp"

namespace hft {
template <typename T>
using Expected = std::expected<T, StatusCode>;

template <typename T>
using Optional = std::optional<T>;

template <typename T, typename Event, void (T::*func)(const Event &)>
static void crefHandlerBridge(void *ctx, const Event &event) {
  (static_cast<T *>(ctx)->*func)(event);
}

template <typename T, typename Event, void (T::*func)(Event &&)>
static void moveHandlerBridge(void *ctx, Event &&event) {
  (static_cast<T *>(ctx)->*func)(std::move(event));
}

template <typename T, void (T::*func)()>
static void callbackBridge(void *ctx) {
  (static_cast<T *>(ctx)->*func)();
}

template <typename Event>
struct CRefHandler {
  using FnPtr = void (*)(void *, const Event &);

  void *ctx{nullptr};
  FnPtr clb{nullptr};

  template <typename T, void (T::*func)(const Event &)>
  static constexpr CRefHandler bind(T *ctx) {
    return {ctx, &crefHandlerBridge<T, Event, func>};
  }

  inline void operator()(const Event &ev) const { clb(ctx, ev); }
  explicit operator bool() const { return ctx != nullptr && clb != nullptr; }
};

template <typename Event>
struct MoveHandler {
  using FnPtr = void (*)(void *, Event &&);

  void *ctx{nullptr};
  FnPtr clb{nullptr};

  template <typename T, void (T::*func)(Event &&)>
  static constexpr MoveHandler bind(T *ctx) {
    return {ctx, &moveHandlerBridge<T, Event, func>};
  }

  inline void operator()(Event &&ev) const { clb(ctx, std::move(ev)); }
  explicit operator bool() const { return ctx != nullptr && clb != nullptr; }
};

struct Callback {
  using FnPtr = void (*)(void *);

  void *ctx{nullptr};
  FnPtr clb{nullptr};

  template <typename T, void (T::*func)()>
  static constexpr Callback bind(T *ctx) {
    return {ctx, &callbackBridge<T, func>};
  }

  inline void operator()() const { clb(ctx); }
  explicit operator bool() const { return ctx != nullptr && clb != nullptr; }
};

using StdCallback = std::function<void()>;

template <typename ArgType>
using StdCRefHandler = std::function<void(const ArgType &)>;

} // namespace hft

#endif // HFT_COMMON_FUNCTIONALTYPES_HPP
