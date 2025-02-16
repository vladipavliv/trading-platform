/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_BALANCING_EVENTSINK_HPP
#define HFT_BALANCING_EVENTSINK_HPP

#include <functional>
#include <ranges>
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

template <typename EventType>
struct KeyExtractor;

template <>
struct KeyExtractor<Order> {
  static const Ticker &key(const Order &order) { return order.ticker; }
};

/**
 * @brief Distributes events by Ticker across N threads
 * @details Performs balancing based on event traffic and threads load
 * Whenever rebalancing is needed simply assign new thread id to a Ticker
 * Not quite sure if its any good of an idea, it would require significant load to test and check
 * Maybe there are better ideas on the market, but it's cool and its my so lets go
 * TODO(this) Profile the stuff
 * @bug Just realized, balancing would require additional management of order book
 * Say thread 1 is busy and it processes orders of a certain ticker, then rebalance happens
 * and ticker gets assigned to another thread which immediately picks up new orders
 * while previous thread is still in work. Probably some book merging could solve the issue
 */
template <size_t ThreadsCount, typename... EventTypes>
class BalancingEventSink {
  /**
   * @brief Links event with the thread id
   * @details Need alignment cause when rebalancing would happen one thread could
   * increment the counter while another thread changes the threadId
   */
  struct alignas(CACHE_LINE_SIZE) ControlBlock {
    mutable std::atomic<ThreadId> threadId;
    Padding<ThreadId> p1; // Plenty of space
    mutable std::atomic<size_t> eventCounter;
    Padding<ThreadId> p2; // For more statistics
  };

  struct alignas(CACHE_LINE_SIZE) ThreadEventCounter {
    size_t value{0};
    Padding<size_t> p;
  };

  /**
   * @brief All const for thread safety
   */
  using ControlBlockMap = const std::unordered_map<Ticker, const ControlBlock>;

public:
  BalancingEventSink(const std::array<std::set<Ticker>, ThreadsCount> &tickerGroups) {
    // First create all the LFQs, evety thread would have its own tuple of LFQs with blackjack
    mEventQueues.reserve(ThreadsCount);
    std::generate_n(std::back_inserter(mEventQueues), ThreadsCount,
                    [this]() { return createLFQueueTuple<EventTypes...>(EVENT_QUEUE_SIZE); });
    // Now initialize control blocks
    for (auto &&[index, value] : std::views::enumerate(tickerGroups)) {
      std::string tickers;
      for (auto &ticker : value) {
        mControlBlockMap[ticker] = ControlBlock{index, 0};
        tickers += std::string(ticker.begin(), ticker.end()) + " ";
      }
      spdlog::debug("Tickers {} are assigned to core {}", index);
    }
  }

  ~BalancingEventSink() { stop(); }

  void start() {
    for (size_t i = 0; i < ThreadsCount; ++i) {
      mThreads[i] = std::thread([this, i] {
        spdlog::debug("Started {} thread", i);
        utils::pinThreadToCore(i * 2);
        utils::setTheadRealTime();
        mThreadId = i;
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
    if (!mControlBlockMap.contains(KeyExtractor<EventType>::key(event))) {
      spdlog::error("Unknown ticker");
      return;
    }
    const auto &block = mControlBlockMap[KeyExtractor<EventType>::key(event.ticker)];
    auto &queue = getQueue<EventType>(block.threadId);
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

  std::vector<std::map<Ticker, size_t>> getStats() {
    std::vector<std::map<Ticker, size_t>> result;
    result.reserve(ThreadsCount);
    for (auto &&[index, value] : std::views::enumerate(mControlBlockMap)) {
      result[index][value.first] = value.second.eventCounter.load();
    }
  }

  void rebalance(const Ticker &ticker, ThreadId newThreadId) {
    assert(newThreadId < ThreadsCount);
    // For now just manual rebalancing, all new events would be handled by the %newThreadId%
    auto &block = mControlBlockMap[ticker];
    block.threadId.store(newThreadId);
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

  std::array<std::thread, ThreadsCount> mThreads;
  ControlBlockMap mControlBlockMap;
  static thread_local ThreadId mThreadId;
  std::atomic_bool mStop{false};
};

template <size_t ThreadCount, typename... EventTypes>
thread_local ThreadId BalancingEventSink<ThreadCount, EventTypes...>::mThreadId = 0;

} // namespace hft

#endif // HFT_BALANCING_EVENTSINK_HPP