/**
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

template <typename... EventTypes>
class IoSink {
public:
  IoSink() : mCtxGuard{mCtx} {}
  ~IoSink() {
    for (auto &thread : mThreads) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

  void start() {
    if (Config::cfg.coresIo.empty()) {
      spdlog::error("No cores provided");
      assert(false);
      return;
    }
    mThreads.reserve(Config::cfg.coresIo.size());
    for (size_t i = 0; i < Config::cfg.coresIo.size(); ++i) {
      mThreads.emplace_back([this, i]() {
        spdlog::debug("Started Io thread on the core ID:{}", Config::cfg.coresIo[i]);
        utils::pinThreadToCore(Config::cfg.coresIo[i]);
        utils::setTheadRealTime();
        mCtx.run();
      });
    }
  }

  template <typename EventType>
    requires(std::disjunction_v<std::is_same<EventType, EventTypes>...>)
  void setHandler(std::function<void(const EventType &)> &&handler) {
    constexpr auto index = getEventIndex<EventType, EventTypes...>();
    std::get<index>(mHandlers) = std::move(handler);
  }

  template <typename EventType>
    requires(std::disjunction_v<std::is_same<EventType, EventTypes>...>)
  void post(const EventType &event) {
    constexpr auto index = getEventIndex<EventType, EventTypes...>();
    std::get<index>(mHandlers)(event);
  }

  void stop() { mCtx.stop(); }
  IoContext &ctx() { return mCtx; }

private:
  template <typename EventType>
  std::function<void(const EventType &)> &getHandler() {
    constexpr auto index = getEventIndex<EventType, EventTypes...>();
    return std::get<index>(mHandlers);
  }

private:
  template <typename EventType, typename... Types>
  static constexpr size_t getEventIndex() {
    return indexOf<EventType, Types...>();
  }

  template <typename EventType, typename First, typename... Rest>
  static constexpr size_t indexOf() {
    if constexpr (std::is_same_v<EventType, First>) {
      return 0;
    } else if constexpr (sizeof...(Rest) > 0) {
      return 1 + indexOf<EventType, Rest...>();
    } else {
      static_assert(sizeof...(Rest) > 0, "EventType not found in EventTypes pack.");
      return -1;
    }
  }

  IoContext mCtx;
  ContextGuard mCtxGuard;

  std::tuple<std::function<void(const EventTypes)>...> mHandlers;
  std::vector<std::thread> mThreads;
};

} // namespace hft

#endif // HFT_COMMON_IOSINK_HPP