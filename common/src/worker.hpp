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
      : mGuard{boost::asio::make_work_guard(mIoCtx)}, mThread{[this, id]() {
          try {
            utils::setTheadRealTime();
            utils::pinThreadToCore(Config::cfg.coreIds[id]);
            mIoCtx.run();
          } catch (const std::exception &e) {
            Logger::monitorLogger->error("Exception in worker thread {}", e.what());
          }
        }} {}

  ~Worker() {
    stop();
    if (mThread.joinable()) {
      mThread.join();
    }
  }

  void stop() { mIoCtx.stop(); }
  void post(Callback job) { mIoCtx.post(std::move(job)); }

private:
  IoContext mIoCtx;
  ContextGuard mGuard;
  Thread mThread;
};

} // namespace hft

#endif // HFT_COMMON_WORKER_HPP