/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_EVENTSINK_HPP
#define HFT_EVENTSINK_HPP

#include <boost/lockfree/queue.hpp>
#include <functional>
#include <spdlog/spdlog.h>
#include <thread>

#include "market_types.hpp"
#include "network_types.hpp"
#include "padded_counter.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft {

template <uint8_t ThreadCount, uint16_t QueueSize, typename... EventTypes>
class EventSink {
public:
  EventSink() = default;
  ~EventSink() { stop(); }

  void start() {
    for (size_t i = 0; i < ThreadCount; ++i) {
      mThreads[i] = std::thread([this, i] {
        mThreadIndex = i;
        spdlog::info("Thread {} started", i);
        processEvents();
      });
    }
  }

  void stop() {
    if (mStop.load()) {
      return;
    }
    mStop.store(true);
    for (auto &thread : mThreads) {
      if (thread.joinable()) {
        thread.join();
      }
    }
    for (size_t i = 0; i < mCounters.size(); ++i) {
      spdlog::debug("Thread {} processed {} events", i, mCounters[i].value.load());
    }
  }

  template <typename EventType>
  void registerHandler(std::function<void(EventType &&)> &&handler) {
    getHandler<EventType>() = std::forward<std::function<void(EventType &&)>>(handler);
  }

  template <typename EventType>
  void post(const EventType &event) {
    static_assert(contains<EventType>(), "Type is not defined in the Sink");
    auto &queue = getQueue<EventType>();
    while (!queue.push(event)) {
      spdlog::debug("Failed to post: {}", utils::getTypeName<EventType>());
      std::this_thread::yield();
    }
  }

private:
  void processEvents() {
    while (!mStop.load()) {
      processQueue<EventTypes...>();
      std::this_thread::yield();
    }
  }

  template <typename EventType>
  void processQueue() {
    auto &queue = getQueue<EventType>();
    EventType event;
    while (queue.pop(event) && !mStop.load()) {
      handleEvent(std::move(event));
    }
  }

  template <typename EventType>
  void handleEvent(EventType &&event) {
    auto &handler = getHandler<EventType>();
    if (handler) {
      handler(std::forward<EventType>(event));
      mCounters[mThreadIndex].value.fetch_add(1, std::memory_order_relaxed);
    } else {
      spdlog::error("Handler not set for the type: {}", utils::getTypeName<EventType>());
    }
  }

  template <typename EventType>
  boost::lockfree::queue<EventType> &getQueue() {
    return std::get<boost::lockfree::queue<EventType>>(mEventQueues);
  }

  template <typename EventType>
  std::function<void(EventType &&)> &getHandler() {
    return std::get<std::function<void(EventType &&)>>(mEventHandlers);
  }

  template <typename Type>
  static constexpr bool contains() {
    return (std::is_same<Type, EventTypes>::value || ...);
  }

private:
  std::tuple<boost::lockfree::queue<EventTypes>...> mEventQueues{QueueSize};
  std::tuple<std::function<void(EventTypes &&)>...> mEventHandlers;

  std::array<std::thread, ThreadCount> mThreads;
  std::array<PaddedCounter, ThreadCount> mCounters;
  std::atomic_bool mStop{false};

  static thread_local uint8_t mThreadIndex;
};

template <uint8_t ThreadCount, uint16_t QueueSize, typename... EventTypes>
thread_local uint8_t EventSink<ThreadCount, QueueSize, EventTypes...>::mThreadIndex = 0;

} // namespace hft

#endif // HFT_SERVER_