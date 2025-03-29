/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_WORKER_HPP
#define HFT_COMMON_WORKER_HPP

#include <memory>

#include "boost_types.hpp"
#include "config/config.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Worker thread with an io_context.
 * Performs significantly better then a manual lock free queue
 */
class Worker {
public:
  using UPtr = std::unique_ptr<Worker>;

  IoContext ioCtx;

  Worker(ThreadId id)
      : guard_{MakeGuard(ioCtx.get_executor())}, thread_{[this, id]() {
          try {
            auto coreId = Config::cfg.coresApp[id];
            utils::setTheadRealTime(coreId);
            utils::pinThreadToCore(coreId);
            ioCtx.run();
          } catch (const std::exception &e) {
            Logger::monitorLogger->error("Exception in worker thread {}", e.what());
          }
        }} {}

  ~Worker() {
    ioCtx.stop();
    if (thread_.joinable()) {
      thread_.join();
    }
  }

private:
  ContextGuard guard_;
  Thread thread_;
};

} // namespace hft

#endif // HFT_COMMON_WORKER_HPP
