/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_BALANCING_EVENTSINK_HPP
#define HFT_BALANCING_EVENTSINK_HPP

#include <functional>
#include <spdlog/spdlog.h>
#include <thread>
#include <unordered_map>

#include "boost_types.hpp"
#include "constants.hpp"
#include "event_fd.hpp"
#include "market_types.hpp"
#include "network_types.hpp"
#include "template_types.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Distributes events by Ticker across N threads
 * @details Every thread has its own set of queues. Tickers are distributed among threads
 * and each thread works with order books synchronously. Rebalancing is suggested to be done
 * via data aggregator, that holds a map Ticker -> ThreadId, so it can be done atomicly
 * by redirecting to another ThreadId, letting previous thread process its incoming orders
 * meanwhile working on another queues, once OrderBook is free, new thread takes over
 */
template <typename LoadBalancer, typename... EventTypes>
class PartitionEventSink {
public:
  using Balancer = LoadBalancer;

  PartitionEventSink() {
    const auto threads = Config().cfg.coresApp.size();
    mEventQueues.reserve(threads);
    std::generate_n(std::back_inserter(mEventQueues), threads,
                    [this]() { return createLFQueueTuple<EventTypes...>(LFQ_SIZE); });
  }

  ~PartitionEventSink() { stop(); }

  void start() {
    if (Config::cfg.coresApp.empty()) {
      spdlog::error("No cores provided");
      assert(false);
      return;
    }
    mThreads.reserve(Config::cfg.coresApp.size());
    for (size_t i = 0; i < Config().cfg.coresApp.size(); ++i) {
      mThreads.emplace_back([this, i] {
        try {
          ThreadId core = Config().cfg.coresApp[i];
          spdlog::trace("Started Worker thread on the core: {}", core);
          utils::pinThreadToCore(core);
          utils::setTheadRealTime();
          mThreadId = i;
          processEvents();
        } catch (const std::exception &e) {
          spdlog::error(e.what());
        }
      });
    }
  }

  void stop() {
    if (mStop.load()) {
      return;
    }
    mEFd.notify();
    mStop.store(true);
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
  void post(const EventType &event) {
    auto id = LoadBalancer::getWorkerId(event);
    if (id == std::numeric_limits<uint8_t>::max()) {
      spdlog::error("Ticker not found: {}", utils::toString(event));
      return;
    }
    auto &queue = getQueue<EventType>(id);
    while (!queue.push(event)) {
      std::this_thread::yield();
    }
    mEFd.notify();
  }

  template <typename EventType>
  void post(Span<EventType> events) {
    for (auto &event : events) {
      post<EventType>(event);
    }
  }

private:
  void processEvents() {
    mEFd.wait();
    while (!mStop.load()) {
      (processQueue<EventTypes>(mThreadId), ...);
      mEFd.wait([this]() { return !isTupleEmpty(mEventQueues[mThreadId]); });
    }
  }

  template <typename EventType>
  void processQueue(ThreadId id) {
    auto &queue = getQueue<EventType>(id);
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
  LFQueue<EventType> &getQueue(ThreadId id) {
    auto &tupl = mEventQueues[id];
    return *std::get<UPtrLFQueue<EventType>>(tupl);
  }

  template <typename EventType>
  SpanHandler<EventType> &getHandler() {
    return std::get<SpanHandler<EventType>>(mEventHandlers);
  }

private:
  // Every thread has its own tuple with queues for events
  std::vector<std::tuple<UPtrLFQueue<EventTypes>...>> mEventQueues;
  std::tuple<SpanHandler<EventTypes>...> mEventHandlers;
  std::vector<std::thread> mThreads;

  static thread_local ThreadId mThreadId;
  EventFd mEFd;
  std::atomic_bool mStop{false};
};

template <typename LoadBalancer, typename... EventTypes>
thread_local ThreadId PartitionEventSink<LoadBalancer, EventTypes...>::mThreadId = 0;

} // namespace hft

#endif // HFT_BALANCING_EVENTSINK_HPP