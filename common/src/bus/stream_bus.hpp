/**
 * @author Vladimir Pavliv
 * @date 2025-08-08
 */

#ifndef HFT_COMMON_STREAMBUS_HPP
#define HFT_COMMON_STREAMBUS_HPP

#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>

#include "config/config.hpp"
#include "constants.hpp"
#include "containers/vyukov_mpmc.hpp"
#include "ctx_runner.hpp"
#include "execution.hpp"
#include "primitive_types.hpp"

namespace hft {

/**
 * @brief Bus for heavy workload with no responsiveness guarantees, optimized for maximum post speed
 * @details Telemetry stream is very intense. To mitigate effect on the hot path,
 * custom optimized lock-free queue is used instead of io_context queue
 * Benchmarks StreamBus::post vs SystemBus::post => 14.3ns vs 52.1 ns
 * This is not used in hot paths cause io_context is optimized not only for producing, but
 * also for consuming. Here we dont care as much for consume side, cause we optimizing for
 * minimal effect on a hot path. In the hot path both sides matter
 */
template <size_t Capacity, typename... Events>
class StreamBus {
  static constexpr size_t EventCount = sizeof...(Events);

  template <typename Event>
  using Lfq = VyukovMPMC<Event, Capacity>;

  template <typename Event>
  using UPtrLfq = UPtr<Lfq<Event>>;

public:
  template <typename Event>
  static constexpr bool Routed = utils::contains<Event, Events...>;

  explicit StreamBus(SystemBus &bus)
      : rate_{Milliseconds(Config::get<size_t>("rates.telemetry_ms"))},
        queues_{std::make_tuple(std::make_unique<Lfq<Events>>()...)}, handlers_{},
        runner_{ErrorBus{bus}}, timer_{runner_.ioCtx} {}

  explicit StreamBus(CoreId coreId, SystemBus &bus)
      : rate_{Milliseconds(Config::get<size_t>("rates.telemetry_ms"))},
        queues_{std::make_tuple(std::make_unique<Lfq<Events>>()...)}, handlers_{},
        runner_{0, coreId, ErrorBus{bus}}, timer_{runner_.ioCtx} {}

  inline IoCtx &streamIoCtx() { return runner_.ioCtx; }

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
    auto &handler = std::get<CRefHandler<Event>>(handlers_);

    if (!handler) {
      LOG_ERROR("Handler is not set for the type {}", typeid(Event).name());
      return false;
    }

    size_t pushRetry{0};
    while (!queue->push(event)) {
      if (++pushRetry > BUSY_WAIT_CYCLES) {
        LOG_ERROR("StreamBus event queue is full for {}", typeid(Event).name());
        return false;
      }
      asm volatile("pause" ::: "memory");
    }
    return true;
  }

  void run() {
    if (running_) {
      LOG_ERROR("StreamBus is already running");
      return;
    }
    LOG_DEBUG("Running StreamBus");
    running_ = true;
    runner_.run();
    scheduleStatsTimer();
  }

  void stop() {
    if (running_) {
      running_ = false;
      timer_.cancel();
      runner_.stop();
    }
  }

private:
  void scheduleStatsTimer() {
    if (!running_) {
      return;
    }
    timer_.expires_after(rate_);
    timer_.async_wait([this](BoostErrorCode code) {
      if (code) {
        if (code != ERR_ABORTED) {
          LOG_ERROR("{}", code.message());
        }
        return;
      }
      while (!empty()) {
        (process<Events>(), ...);
      }
      asm volatile("pause" ::: "memory");
      scheduleStatsTimer();
    });
  }

  template <typename Event>
  void process() {
    auto &queue = std::get<UPtrLfq<Event>>(queues_);
    auto &handler = std::get<CRefHandler<Event>>(handlers_);
    if (!handler) {
      return;
    }
    Event event;
    while (queue->pop(event)) {
      handler(event);
    }
  }

  bool empty() const { return (std::get<UPtrLfq<Events>>(queues_)->empty() && ...); }

  StreamBus(const StreamBus &) = delete;
  StreamBus(StreamBus &&) = delete;
  StreamBus &operator=(const StreamBus &) = delete;
  StreamBus &operator=(const StreamBus &&) = delete;

private:
  std::atomic_bool running_{false};
  const Milliseconds rate_;

  std::tuple<UPtrLfq<Events>...> queues_;
  std::tuple<CRefHandler<Events>...> handlers_;

  CtxRunner runner_;
  SteadyTimer timer_;
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