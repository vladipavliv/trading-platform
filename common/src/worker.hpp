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

class Worker {
public:
  using UPtr = std::unique_ptr<Worker>;

  Worker(ThreadId id)
      : guard_{boost::asio::make_work_guard(ioCtx)}, thread_{[this, id]() {
          try {
            utils::setTheadRealTime();
            utils::pinThreadToCore(Config::cfg.coreIds[id]);
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

  IoContext ioCtx;

private:
  ContextGuard guard_;
  Thread thread_;
};

} // namespace hft

#endif // HFT_COMMON_WORKER_HPP
