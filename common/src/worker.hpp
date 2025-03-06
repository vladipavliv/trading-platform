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
      : guard_{boost::asio::make_work_guard(ioCtx_)}, thread_{[this, id]() {
          try {
            utils::setTheadRealTime();
            utils::pinThreadToCore(Config::cfg.coreIds[id]);
            ioCtx_.run();
          } catch (const std::exception &e) {
            Logger::monitorLogger->error("Exception in worker thread {}", e.what());
          }
        }} {}

  ~Worker() {
    stop();
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  void stop() { ioCtx_.stop(); }
  void post(Callback job) { ioCtx_.post(std::move(job)); }

private:
  IoContext ioCtx_;
  ContextGuard guard_;
  Thread thread_;
};

} // namespace hft

#endif // HFT_COMMON_WORKER_HPP