/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_BUFFEREDIOSINK_HPP
#define HFT_COMMON_BUFFEREDIOSINK_HPP

#include <spdlog/spdlog.h>

#include "boost_types.hpp"
#include "comparators.hpp"
#include "config/config.hpp"
#include "network_types.hpp"
#include "template_types.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Buffers incoming events in lock free queue before io threads can pick them up
 * I was concerned about the 5% of 1ms-50ms spikes, and decided to try this batch approach
 * Did not pay off. Last performance check:
 * [20:15:48.671] [I] RTT [1us|100us|1ms]  91.15% avg:30us  4.02% avg:134us  4.83% avg:25ms
 * [20:15:48.219] [I] [open|total]: 121978|186330 RPS:9551
 */
template <typename... EventTypes>
class BufferIoSink {
  static constexpr size_t TypeCount = sizeof...(EventTypes);

public:
  BufferIoSink()
      : mCtxGuard{mCtx.get_executor()}, mEventQueues(createLFQueueTuple<EventTypes...>(LFQ_SIZE)) {}

  ~BufferIoSink() {
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
    if (events.empty()) {
      return;
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

#endif // HFT_COMMON_BUFFEREDIOSINK_HPP