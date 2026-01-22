/**
 * @author Vladimir Pavliv
 * @date 2025-08-08
 */

#ifndef HFT_COMMON_STREAMBUS_HPP
#define HFT_COMMON_STREAMBUS_HPP

#include "config/config.hpp"
#include "constants.hpp"
#include "containers/sequenced_spsc.hpp"
#include "containers/vyukov_mpmc.hpp"
#include "execution.hpp"
#include "primitive_types.hpp"
#include "runner/ctx_runner.hpp"
#include "utils/spin_wait.hpp"
#include "utils/sync_utils.hpp"

namespace hft {

/**
 * @brief Currently not in use
 * Telemetry stream is intense, and it's effect on the hot path must be minimized
 * SystemBus handles important events so it should be responsive too
 * So telemetry needs a separate bus, no need for responsiveness guarantees, maximum post speed
 * As telemetry is currently streamed via shared memory, no need for a special bus
 */
template <size_t Capacity, typename... Events>
class StreamBus {
  static constexpr size_t EventCount = sizeof...(Events);

  template <typename Event>
  using Lfq = VyukovMPMC<Event, LFQ_CAPACITY * 16>;

  template <typename Event>
  using UPtrLfq = UPtr<Lfq<Event>>;

public:
  template <typename Event>
  static constexpr bool Routed = utils::contains<Event, Events...>;

  explicit StreamBus(SystemBus &bus)
      : queues_{std::make_tuple(std::make_unique<Lfq<Events>>()...)}, handlers_{} {}

  explicit StreamBus(CoreId coreId, SystemBus &bus)
      : queues_{std::make_tuple(std::make_unique<Lfq<Events>>()...)}, handlers_{} {}

  template <typename Event>
    requires Routed<Event>
  void subscribe(CRefHandler<Event> &&handler) {
    if (running_) {
      throw std::runtime_error("StreamBus subscribe called after start");
    }
    auto &handlerRef = std::get<CRefHandler<Event>>(handlers_);
    if (handlerRef) {
      LOG_ERROR("Handler is already registered for the type {}", typeid(Event).name());
    } else {
      handlerRef = std::move(handler);
    }
  }

  template <typename Event>
    requires Routed<Event>
  inline bool post(CRef<Event> event) {
    if (!running_) {
      LOG_ERROR_SYSTEM("StreamBus is not running");
      return false;
    }
    auto &queue = std::get<UPtrLfq<Event>>(queues_);

    SpinWait waiter;
    while (!queue->push(event)) {
      if (!++waiter) {
        LOG_ERROR_SYSTEM("StreamBus event queue is full for {}", typeid(Event).name());
        return false;
      }
    }
    notify();
    return true;
  }

  void run() {
    if (running_) {
      LOG_ERROR_SYSTEM("StreamBus is already running");
      return;
    }
    LOG_INFO_SYSTEM("Starting StreamBus");
    runner_ = std::jthread([this]() {
      running_ = true;

      SpinWait waiter;
      while (running_.load(std::memory_order_acquire)) {
        if ((process<Events>() | ...)) {
          waiter.reset();
          continue;
        }
        if (!++waiter) {
          waitForData();
          waiter.reset();
        }
      }
    });
  }

  void stop() {
    running_.store(false, std::memory_order_release);
    futex_.fetch_add(1, std::memory_order_release);
    utils::futexWake(futex_);
  }

private:
  template <typename Event>
  bool process() {
    auto &queue = std::get<UPtrLfq<Event>>(queues_);
    auto &handler = std::get<CRefHandler<Event>>(handlers_);
    if (!handler) {
      return false;
    }
    Event event;
    if (queue->pop(event)) {
      do {
        handler(event);
      } while (queue->pop(event));
      return true;
    }
    return false;
  }

  void waitForData() {
    LOG_DEBUG("Wait for data");
    uint32_t futexVal = futex_.load(std::memory_order_acquire);
    sleeping_.store(true, std::memory_order_release);
    utils::futexWait(futex_, futexVal);
    sleeping_.store(false, std::memory_order_release);
  }

  void notify() {
    if (sleeping_.load(std::memory_order_acquire)) {
      futex_.fetch_add(1, std::memory_order_release);
      utils::futexWake(futex_);
    }
  }

  bool empty() const { return (std::get<UPtrLfq<Events>>(queues_)->empty() && ...); }

  StreamBus(const StreamBus &) = delete;
  StreamBus(StreamBus &&) = delete;
  StreamBus &operator=(const StreamBus &) = delete;
  StreamBus &operator=(const StreamBus &&) = delete;

private:
  alignas(64) AtomicBool running_{false};
  alignas(64) AtomicBool sleeping_{false};
  alignas(64) AtomicUInt32 futex_;

  std::tuple<UPtrLfq<Events>...> queues_;
  std::tuple<CRefHandler<Events>...> handlers_;

  std::jthread runner_;
};

template <size_t Capacity>
class StreamBus<Capacity> {
public:
  static constexpr size_t capacity = Capacity;

  template <typename Event>
  static constexpr bool Routed = false;

  explicit StreamBus(SystemBus &) {}
  explicit StreamBus(CoreId, SystemBus &) {}

  inline IoCtx *streamIoCtx() {
    LOG_ERROR("Use of uninitialized StreamBus");
    return nullptr;
  }

  template <typename Event>
  void subscribe(CRefHandler<Event> &&) {
    LOG_ERROR("Use of uninitialized StreamBus");
  }

  template <typename Event>
  inline bool post(CRef<Event>) {
    LOG_ERROR("Use of uninitialized StreamBus");
    return false;
  }

  void run() {}
  void stop() {}
};

} // namespace hft

#endif // HFT_COMMON_STREAMBUS_HPP
