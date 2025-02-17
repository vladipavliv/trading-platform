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
#include "market_types.hpp"
#include "network_types.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Distributes events by Ticker across N threads
 * @details Every thread works with its own set of tickers. Better for cache locality,
 * significantly simpler code, the only issue i see is rebalancing. For that we map
 * Ticker -> ThreadId. All tickers are well known and never change so synchronizing the whole map
 * is not needed. When we switch the thread id we can have the order book for thet ticker
 * marked 'in transition' untill old thread processes its incoming orders
 * and new thread takes over. Maybe unprocessed orders could be transferred to a new threads buffer
 * maybe some book merging could take place so both threads can proceed.
 *
 */
template <typename LoadBalancer, typename... EventTypes>
class BalancingEventSink {
public:
  using Balancer = LoadBalancer;

  BalancingEventSink() {
    const auto threads = Config().cfg.coresApp.size();
    mEventQueues.reserve(threads);
    std::generate_n(std::back_inserter(mEventQueues), threads,
                    [this]() { return createLFQueueTuple<EventTypes...>(EVENT_QUEUE_SIZE); });
  }

  ~BalancingEventSink() { stop(); }

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
          spdlog::debug("Started Worker thread on the core: {}", core);
          utils::pinThreadToCore(core);
          utils::setTheadRealTime();
          mThreadId = i;
          processEvents();
        } catch (const std::exception &e) {
          std::cerr << e.what() << '\n';
        }
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
    auto id = LoadBalancer::getWorkerId(event);
    if (id == std::numeric_limits<uint8_t>::max()) {
      // Ticker not found
      spdlog::error("Ticker not found: {}", utils::toString(event));
      return;
    }
    auto &queue = getQueue<EventType>(id);
    while (!queue.push(event)) {
      std::this_thread::yield();
    }
  }

  template <typename EventType>
    requires(std::disjunction_v<std::is_same<EventType, EventTypes>...>)
  void post(const std::vector<EventType> &events) {
    for (auto &event : events) {
      post<EventType>(event);
    }
  }

private:
  void processEvents() {
    while (!mStop.load()) {
      (processQueue<EventTypes>(mThreadId), ...);
    }
  }

  template <typename EventType>
  void processQueue(ThreadId id) {
    auto &queue = getQueue<EventType>(id);
    queue.consume_all(getHandler<EventType>());
    std::this_thread::yield();
  }

  template <typename EventType>
  LFQueue<EventType> &getQueue(ThreadId id) {
    auto &tupl = mEventQueues[id];
    return *std::get<UPtrLFQueue<EventType>>(tupl);
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

  std::vector<std::tuple<UPtrLFQueue<EventTypes>...>> mEventQueues;
  std::tuple<CRefHandler<EventTypes>...> mEventHandlers;
  std::vector<std::thread> mThreads;

  static thread_local ThreadId mThreadId;
  std::atomic_bool mStop{false};
};

template <typename LoadBalancer, typename... EventTypes>
thread_local ThreadId BalancingEventSink<LoadBalancer, EventTypes...>::mThreadId = 0;

} // namespace hft

#endif // HFT_BALANCING_EVENTSINK_HPP