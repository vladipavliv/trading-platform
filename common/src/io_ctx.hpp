/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_IOCTXWRAPPER_HPP
#define HFT_COMMON_IOCTXWRAPPER_HPP

#include "boost_types.hpp"

namespace hft {

/**
 * @brief Helper io_ctx wrapper
 */
class IoCtx {
public:
  BoostIoCtx ctx;

  IoCtx() : guard_{MakeGuard(ctx.get_executor())} {}

  operator BoostIoCtx &() { return ctx; }

  void run() { ctx.run(); }

  void stop() { ctx.stop(); }

  template <typename Func>
  void post(Func &&func) {
    boost::asio::post(ctx, std::forward<Func>(func));
  }

private:
  ContextGuard guard_;
};

} // namespace hft

#endif // HFT_COMMON_IOCTXWRAPPER_HPP
