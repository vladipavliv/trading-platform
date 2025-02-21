/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_EVENTSINK_HPP
#define HFT_EVENTSINK_HPP

#include <functional>
#include <spdlog/spdlog.h>
#include <thread>

#include "boost_types.hpp"
#include "market_types.hpp"
#include "network_types.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Processes events on N threads via lock-free queues, whatever thread
 * picks up the message gets to process it. Incompatible with current data aggregator
 * implementation as the latter relies on the pool for concurrency
 */
template <typename... EventTypes>
class PoolEventSink {
public:
  PoolEventSink() : mEventQueues(createLFQueueTuple<EventTypes...>(LFQ_SIZE)) {}
  ~PoolEventSink() { stop(); }

  void start() {
    if (Config::cfg.coresApp.empty()) {
      spdlog::error("No cores provided");
      return;
    }
    mThreads.reserve(Config::cfg.coresApp.size());
    for (size_t i = 0; i < Config::cfg.coresApp.size(); ++i) {
      mThreads.emplace_back([this, i] {
        spdlog::trace("Started Worker thread on the core: {}", Config::cfg.coresApp[i]);
        utils::pinThreadToCore(Config::cfg.coresApp[i]);
        utils::setTheadRealTime();
        processEvents();
      });
    }
  }

  void stop() {
    if (mStop.load()) {
      return;
    }
    mStop.store(true);
    mEFd.notify();
    for (auto &thread : mThreads) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

  template <typename EventType>
  void setHandler(SpanHandler<EventType> &&handler) {
    getHandler<EventType>() = std::move(handler);
  }

  template <typename EventType>
  void post(Span<EventType> events) {
    auto &queue = getQueue<EventType>();
    for (auto &event : events) {
      while (!queue.push(event)) {
        std::this_thread::yield();
      }
    }
    mEFd.notify();
  }

private:
  void processEvents() {
    mEFd.wait();
    while (!mStop.load()) {
      (processQueue<EventTypes>(), ...);
      mEFd.wait([this]() { return !isTupleEmpty(mEventQueues); });
    }
  }

  template <typename EventType>
  void processQueue() {
    auto &queue = getQueue<EventType>();
    if (queue.empty()) {
      return;
    }
    std::vector<EventType> buffer;
    buffer.reserve(LFQ_POP_LIMIT);
    EventType event;
    while (buffer.size() < LFQ_POP_LIMIT && queue.pop(event)) {
      buffer.emplace_back(event);
    }
    getHandler<EventType>()(Span<EventType>(buffer));
  }

  template <typename EventType>
  LFQueue<EventType> &getQueue() {
    return *std::get<UPtrLFQueue<EventType>>(mEventQueues);
  }

  template <typename EventType>
  SpanHandler<EventType> &getHandler() {
    return std::get<SpanHandler<EventType>>(mEventHandlers);
  }

  template <typename Type>
  static constexpr bool contains() {
    return (std::is_same<Type, EventTypes>::value || ...);
  }

private:
  std::tuple<UPtrLFQueue<EventTypes>...> mEventQueues;
  std::tuple<SpanHandler<EventTypes>...> mEventHandlers;

  EventFd mEFd;
  std::atomic_bool mStop{false};
  static thread_local uint8_t mThreadIndex;

  std::vector<std::thread> mThreads;
};

} // namespace hft

#endif // HFT_EVENTSINK_HPP