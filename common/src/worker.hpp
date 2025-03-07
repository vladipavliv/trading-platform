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

  IoContext ioCtx;

  Worker(ThreadId id)
      : guard_{boost::asio::make_work_guard(ioCtx)}, thread_{[this, id]() {
          try {
            utils::setTheadRealTime();
            utils::pinThreadToCore(Config::cfg.coresApp[id]);
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

  void warmUpStart() {
    if (warmedUp_) {
      return;
    }
    warmUpCycle();
  }

  void warmUpStop() { warmedUp_.store(true, std::memory_order_release); }

private:
  void warmUpCycle() {
    if (warmedUp_.load(std::memory_order_acquire)) {
      return;
    }
    ioCtx.post([this]() {
      utils::coreWarmUpJob();
      warmUpCycle();
    });
  }

  ContextGuard guard_;
  std::atomic_bool warmedUp_{false};
  Thread thread_;
};

} // namespace hft

#endif // HFT_COMMON_WORKER_HPP
