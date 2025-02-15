/**
 * @file
 * @brief
 *
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
#include "padded_counter.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft {

template <typename... EventTypes>
class EventSink {
public:
  EventSink() : mEventQueues(createLFQueueTuple<EventTypes...>(EVENT_Q_SIZE)) {}
  ~EventSink() { stop(); }

  void start() {
    for (size_t i = 0; i < THREADS_APP; ++i) {
      mThreads[i] = std::thread([this, i] {
        utils::setTheadRealTime();
        mThreadIndex = i;
        // spdlog::info("Thread {} started", i);
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
      // spdlog::debug("Thread {} processed {} events", i, mCounters[i].value.load());
    }
  }

  template <typename EventType>
  void setHandler(CRefHandler<EventType> &&handler) {
    getHandler<EventType>() = std::move(handler);
  }

  template <typename EventType>
    requires(std::disjunction_v<std::is_same<EventType, EventTypes>...>)
  void post(const EventType &event) {
    auto &queue = getQueue<EventType>();
    while (!queue.push(event)) {
      std::this_thread::yield();
    }
  }

  template <typename EventType>
    requires(std::disjunction_v<std::is_same<EventType, EventTypes>...>)
  void post(const std::vector<EventType> &events) {
    auto &queue = getQueue<EventType>();
    for (auto &event : events) {
      while (!queue.push(event)) {
        std::this_thread::yield();
      }
    }
  }

private:
  void processEvents() {
    while (!mStop.load()) {
      (processQueue<EventTypes>(), ...);
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
  void handleEvent(const EventType &event) {
    auto &handler = getHandler<EventType>();
    if (handler) {
      handler(event);
      mCounters[mThreadIndex].value.fetch_add(1, std::memory_order_relaxed);
    } else {
      spdlog::error("Handler not set: {}", utils::toString<EventType>(event));
    }
  }

  template <typename EventType>
  LFQueue<EventType> &getQueue() {
    return *std::get<UPtrLFQueue<EventType>>(mEventQueues);
  }

  template <typename EventType>
  std::function<void(const EventType &)> &getHandler() {
    return std::get<CRefHandler<EventType>>(mEventHandlers);
  }

  template <typename Type>
  static constexpr bool contains() {
    return (std::is_same<Type, EventTypes>::value || ...);
  }

private:
  template <typename EventType>
  static UPtrLFQueue<EventType> createLFQueue(std::size_t size) {
    return std::make_unique<LFQueue<EventType>>(size);
  }

  template <typename... TupleTypes>
  static std::tuple<UPtrLFQueue<TupleTypes>...> createLFQueueTuple(std::size_t size) {
    return std::make_tuple(createLFQueue<TupleTypes>(size)...);
  }

  std::tuple<UPtrLFQueue<EventTypes>...> mEventQueues;
  std::tuple<CRefHandler<EventTypes>...> mEventHandlers;

  std::array<std::thread, THREADS_APP> mThreads;
  std::array<PaddedCounter, THREADS_APP> mCounters;
  std::atomic_bool mStop{false};

  static thread_local uint8_t mThreadIndex;
};

template <typename... EventTypes>
thread_local uint8_t EventSink<EventTypes...>::mThreadIndex = 0;

} // namespace hft

#endif // HFT_SERVER_