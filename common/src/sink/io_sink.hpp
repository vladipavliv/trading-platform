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
 * @brief Objects that perform network operations subscribe for events via this sink
 * Last performance check:
 * [20:10:34.002] [I] RTT [1us|100us|1ms]  94.86% avg:26us  0.17% avg:352us  4.96% avg:25ms
 * [20:10:46.468] [I] [open|total]: 306256|479623 RPS:9551
 * Performs slightly better then BufferIoSink
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
          spdlog::debug("Started Io thread {} on the core: {}", i, Config::cfg.coresIo[i]);
          utils::pinThreadToCore(Config::cfg.coresIo[i]);
          utils::setTheadRealTime();
          mCtx.run();
          spdlog::debug("Finished Io thread {}", i);
        } catch (const std::exception &e) {
          spdlog::critical("Exception in Io thread {}", e.what());
        }
      });
    }
    try {
      mCtx.run();
    } catch (const std::exception &e) {
      spdlog::critical("Exception in Io thread {}", e.what());
    }
  }

  template <typename EventType>
  void setHandler(SpanHandler<EventType> &&handler) {
    constexpr auto index = getTypeIndex<EventType, EventTypes...>();
    std::get<index>(mHandlers) = std::move(handler);
  }

  template <typename EventType>
  void post(Span<EventType> event) {
    constexpr auto index = getTypeIndex<EventType, EventTypes...>();
    std::get<index>(mHandlers)(event);
  }

  void stop() { mCtx.stop(); }
  IoContext &ctx() { return mCtx; }

private:
  template <typename EventType>
  SpanHandler<EventType> &getHandler() {
    constexpr auto index = getTypeIndex<EventType, EventTypes...>();
    return std::get<index>(mHandlers);
  }

private:
  IoContext mCtx;
  ContextGuard mCtxGuard;

  std::tuple<SpanHandler<EventTypes>...> mHandlers;
  std::vector<std::thread> mThreads;
};

} // namespace hft

#endif // HFT_COMMON_IOSINK_HPP