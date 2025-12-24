/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_CTXRUNNER_HPP
#define HFT_COMMON_CTXRUNNER_HPP

#include <memory>

#include "boost_types.hpp"
#include "bus/system_bus.hpp"
#include "internal_error.hpp"
#include "logging.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Worker thread with an io_context
 */
class CtxRunner {
public:
  IoCtx ioCtx;

  CtxRunner(ErrorBus &&bus) : guard_{MakeGuard(ioCtx.get_executor())}, bus_{std::move(bus)} {}

  CtxRunner(ThreadId id, ErrorBus &&bus)
      : threadId_{id}, guard_{MakeGuard(ioCtx.get_executor())}, bus_{std::move(bus)} {}

  CtxRunner(ThreadId id, CoreId coreId, ErrorBus &&bus)
      : threadId_{id}, coreId_{coreId}, guard_{MakeGuard(ioCtx.get_executor())},
        bus_{std::move(bus)} {}

  ~CtxRunner() { ioCtx.stop(); }

  void run() {
    if (running_) {
      LOG_ERROR("Worker is already running");
      return;
    }
    running_ = true;
    thread_ = Thread{[this]() {
      try {
        utils::setTheadRealTime();
        String idStr = threadId_.has_value() ? std::to_string(threadId_.value()) : "";
        if (coreId_.has_value()) {
          utils::pinThreadToCore(coreId_.value());
          LOG_INFO_SYSTEM("Starting worker thread {} on the core {}", idStr, coreId_.value());
        } else {
          LOG_INFO_SYSTEM("Starting worker thread {}", idStr);
        }
        ioCtx.run();
      } catch (CRef<std::exception> e) {
        LOG_ERROR_SYSTEM("std::exception in CtxRunner {}", e.what());
        stop();
        bus_.post(InternalError(StatusCode::Error, e.what()));
      } catch (...) {
        LOG_ERROR_SYSTEM("unknown exception in CtxRunner");
        stop();
        bus_.post(InternalError(StatusCode::Error, "unknown exception in CtxRunner"));
      }
    }};
  }

  void stop() {
    ioCtx.stop();
    running_ = false;
  }

private:
  const std::optional<ThreadId> threadId_;
  const std::optional<CoreId> coreId_;

  std::atomic_bool running_{false};

  ErrorBus bus_;
  ContextGuard guard_;
  Thread thread_;
};

} // namespace hft

#endif // HFT_COMMON_CTXRUNNER_HPP
