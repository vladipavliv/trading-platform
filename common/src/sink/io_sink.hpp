/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_IOSINK_HPP
#define HFT_COMMON_IOSINK_HPP

#include <spdlog/spdlog.h>

#include "config/config.hpp"
#include "network_types.hpp"

namespace hft {

class IoSink {
public:
  IoSink() : mCtxGuard{mCtx}, mThreadCount{Config::config().networkThreads} {}
  ~IoSink() {
    for (auto &thread : mThreads) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

  void start() {
    if (mThreadCount == 0) {
      mCtx.run();
    }
    mThreads.reserve(mThreadCount);
    for (size_t i = 0; i < mThreadCount; ++i) {
      mThreads.emplace_back([this, i]() {
        spdlog::debug("Io thread {} started", i);
        try {
          mCtx.run();
        } catch (const std::exception &e) {
          spdlog::error("Exception in Io thread {}: {}", i, e.what());
        }
        spdlog::debug("Io thread {} stopped", i);
      });
    }
  }

  template <typename Callable>
  void post(Callable &&clb) {
    boost::asio::post(mCtx, [clb = std::forward<Callable>(clb)]() mutable { clb(); });
  }

  void stop() { mCtx.stop(); }
  IoContext &ctx() { return mCtx; }

private:
  IoContext mCtx;
  ContextGuard mCtxGuard;

  const uint8_t mThreadCount;
  std::vector<std::thread> mThreads;
};

} // namespace hft

#endif // HFT_COMMON_IOSINK_HPP