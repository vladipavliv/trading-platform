/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_WORKER_HPP
#define HFT_COMMON_WORKER_HPP

#include <memory>

#include "boost_types.hpp"
#include "io_ctx.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Worker thread with an io_context.
 */
class Worker {
public:
  using UPtr = std::unique_ptr<Worker>;

  IoCtx ioCtx;

  Worker(ThreadId id, CoreId coreId)
      : thread_{[this, id, coreId]() {
          try {
            utils::setTheadRealTime(coreId);
            utils::pinThreadToCore(coreId);
            ioCtx.run();
          } catch (const std::exception &e) {
            Logger::monitorLogger->error("Exception in worker thread {}", e.what());
          }
        }} {}

  ~Worker() { ioCtx.stop(); }

private:
  Thread thread_;
};

} // namespace hft

#endif // HFT_COMMON_WORKER_HPP
