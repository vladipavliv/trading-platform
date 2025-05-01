/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_CTXRUNNER_HPP
#define HFT_COMMON_CTXRUNNER_HPP

#include <memory>

#include "boost_types.hpp"
#include "logging.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Worker thread with an io_context
 */
class CtxRunner {
public:
  using UPtr = std::unique_ptr<CtxRunner>;

  IoCtx ioCtx;

  CtxRunner(ThreadId id, bool pinToCore, CoreId coreId = 0)
      : threadId_{id}, pinToCore_{pinToCore}, coreId_{coreId},
        guard_{MakeGuard(ioCtx.get_executor())} {}

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
        if (pinToCore_) {
          utils::pinThreadToCore(coreId_);
          LOG_INFO_SYSTEM("Starting worker thread {} on the core {}", threadId_, coreId_);
        } else {
          LOG_INFO_SYSTEM("Starting worker thread {}", threadId_);
        }
        ioCtx.run();
      } catch (CRef<std::exception> e) {
        LOG_ERROR_SYSTEM("Exception in worker thread {} {}", threadId_, e.what());
      }
    }};
  }

  void stop() { ioCtx.stop(); }

private:
  const ThreadId threadId_;
  const bool pinToCore_;
  const CoreId coreId_;

  bool running_{false};

  ContextGuard guard_;
  Thread thread_;
};

} // namespace hft

#endif // HFT_COMMON_CTXRUNNER_HPP
