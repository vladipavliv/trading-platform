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

template <typename... EventTypes>
class PoolEventSink {
public:
  PoolEventSink() : mEventQueues(createLFQueueTuple<EventTypes...>(EVENT_QUEUE_SIZE)) {}
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
    for (auto &thread : mThreads) {
      if (thread.joinable()) {
        thread.join();
      }
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
    }
  }

  template <typename EventType>
  void processQueue() {
    auto &queue = getQueue<EventType>();
    queue.consume_all(getHandler<EventType>());
    std::this_thread::yield();
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

  std::vector<std::thread> mThreads;
  std::atomic_bool mStop{false};

  static thread_local uint8_t mThreadIndex;
};

} // namespace hft

#endif // HFT_EVENTSINK_HPP