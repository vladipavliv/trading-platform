/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_IOSINK_HPP
#define HFT_COMMON_IOSINK_HPP

#include <spdlog/spdlog.h>

#include "config/config.hpp"
#include "network_types.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Sink for Io operations. Events are being posted not on the IoContext
 * directly to minimize overhead, but rather dispatchers register for events
 * to process. Runs Io context on a number of threads
 */
template <typename... EventTypes>
class IoSink {
public:
  IoSink() : mCtxGuard{mCtx.get_executor()} {}
  ~IoSink() {
    for (auto &thread : mThreads) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

  void start() {
    mThreads.reserve(Config::cfg.coresIo.size());
    for (size_t i = 0; i < Config::cfg.coresIo.size(); ++i) {
      mThreads.emplace_back([this, i]() {
        try {
          spdlog::trace("Started Io thread on the core: {}", Config::cfg.coresIo[i]);
          utils::pinThreadToCore(Config::cfg.coresIo[i]);
          utils::setTheadRealTime();
          mCtx.run();
        } catch (const std::exception &e) {
          spdlog::error(e.what());
        }
      });
    }
    mCtx.run();
  }

  template <typename EventType>
  void setHandler(CRefHandler<EventType> &&handler) {
    constexpr auto index = getTypeIndex<EventType, EventTypes...>();
    std::get<index>(mHandlers) = std::move(handler);
  }

  template <typename EventType>
  void post(const EventType &event) {
    constexpr auto index = getTypeIndex<EventType, EventTypes...>();
    std::get<index>(mHandlers)(event);
  }

  void stop() { mCtx.stop(); }
  IoContext &ctx() { return mCtx; }

private:
  template <typename EventType>
  CRefHandler<EventType> &getHandler() {
    constexpr auto index = getTypeIndex<EventType, EventTypes...>();
    return std::get<index>(mHandlers);
  }

private:
  IoContext mCtx;
  ContextGuard mCtxGuard;

  std::tuple<std::function<void(const EventTypes &)>...> mHandlers;
  std::vector<std::thread> mThreads;
};

} // namespace hft

#endif // HFT_COMMON_IOSINK_HPP