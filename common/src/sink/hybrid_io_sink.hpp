/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_HYBRIDIOSINK_HPP
#define HFT_COMMON_HYBRIDIOSINK_HPP

#include <spdlog/spdlog.h>

#include "boost_types.hpp"
#include "comparators.hpp"
#include "config/config.hpp"
#include "network_types.hpp"
#include "template_types.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief
 */
template <typename... EventTypes>
class HybridIoSink {
  static constexpr size_t TypeCount = sizeof...(EventTypes);

public:
  HybridIoSink()
      : mCtxGuard{mCtx.get_executor()}, mEventQueues(createLFQueueTuple<EventTypes...>(LFQ_SIZE)) {}

  ~HybridIoSink() {
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
  void setHandler(SpanHandler<EventType> &&handler) {
    constexpr auto index = getTypeIndex<EventType, EventTypes...>();
    std::get<index>(mHandlers) = std::move(handler);
  }

  template <typename EventType>
  void post(Span<EventType> events) {
    auto &queue = getQueue<EventType>();
    for (auto &event : events) {
      while (!queue.push(event)) {
        std::this_thread::yield();
      }
    }
    if (!mHandlerPending.load(std::memory_order_relaxed)) {
      mHandlerPending.store(true, std::memory_order_relaxed);
      mCtx.post([this]() { processEvents(); });
    }
  }

  void stop() { mCtx.stop(); }
  IoContext &ctx() { return mCtx; }

private:
  template <typename EventType>
  SpanHandler<EventType> &getHandler() {
    constexpr auto index = getTypeIndex<EventType, EventTypes...>();
    return std::get<index>(mHandlers);
  }

  void processEvents() {
    mHandlerPending.store(false, std::memory_order_relaxed);
    (processQueue<EventTypes>(), ...);
  }

  template <typename EventType>
  void processQueue() {
    auto &queue = getQueue<EventType>();
    if (queue.empty()) {
      return;
    }
    std::vector<EventType> events;
    events.reserve(LFQ_POP_LIMIT);

    EventType event;
    while (events.size() < LFQ_POP_LIMIT && queue.pop(event)) {
      events.emplace_back(std::move(event));
    }
    std::get<SpanHandler<EventType>>(mHandlers)(Span<EventType>(events));
  }

  template <typename EventType>
  LFQueue<EventType> &getQueue() {
    return *std::get<UPtrLFQueue<EventType>>(mEventQueues);
  }

private:
  IoContext mCtx;
  ContextGuard mCtxGuard;

  std::tuple<UPtrLFQueue<EventTypes>...> mEventQueues;
  std::tuple<SpanHandler<EventTypes>...> mHandlers;
  std::atomic_bool mHandlerPending{false};

  std::vector<std::thread> mThreads;
};

} // namespace hft

#endif // HFT_COMMON_HYBRIDIOSINK_HPP