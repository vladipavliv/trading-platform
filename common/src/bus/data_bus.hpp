/**
 * @author Vladimir Pavliv
 * @date 2025-08-08
 */

#ifndef HFT_COMMON_DATABUS_HPP
#define HFT_COMMON_DATABUS_HPP

#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>

#include "config/config.hpp"
#include "ctx_runner.hpp"
#include "types.hpp"
#include "utils/utils.hpp"
#include "vyukov_queue.hpp"

namespace hft {

/**
 * @brief Bus for heavy workload with no responsiveness guarantees, optimized for maximum post speed
 * @details Telemetry stream is very intense. To mitigate effect on the hot path,
 * custom optimized lock-free queue is used instead of io_context queue
 * Benchmarks DataBus::post vs SystemBus::post => 14.3ns vs 52.1 ns
 * This is not used in hot paths cause io_context is optimized not only for producing, but
 * also for consuming. Here we dont care as much for consume side, cause we optimizing for
 * minimal effect on a hot path. In the hot path both sides matter
 */
template <typename... Events>
class DataBus {
  static constexpr size_t QUEUE_SIZE = 1024 * 128;
  static constexpr size_t RETRY_COUNT = 100;

  static constexpr size_t EventCount = sizeof...(Events);

  template <typename Event>
  using Lfq = VyukovQueue<Event, QUEUE_SIZE>;

  template <typename Event>
  using UPtrLfq = UPtr<Lfq<Event>>;

public:
  template <typename Event>
  static constexpr bool Routed = utils::contains<Event, Events...>;

  DataBus()
      : rate_{Config::get<size_t>("rates.telemetry_ms")}, timer_{runner_.ioCtx},
        queues_{std::make_tuple(std::make_unique<Lfq<Events>>()...)}, handlers_{} {}

  inline IoCtx &dataIoCtx() { return runner_.ioCtx; }

  template <typename Event>
    requires Routed<Event>
  void subscribe(CRefHandler<Event> handler) {
    if (running_) {
      throw std::runtime_error("DataBus subscribe called after start");
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
  inline void post(CRef<Event> event) {
    if (!running_) {
      LOG_ERROR_SYSTEM("DataBus is not running");
      return;
    }
    auto &queue = std::get<UPtrLfq<Event>>(queues_);
    auto &handler = std::get<CRefHandler<Event>>(handlers_);

    if (!handler) {
      LOG_ERROR("Handler is not set for the type {}", typeid(Event).name());
      return;
    }

    size_t pushRetry{0};
    while (!queue->push(event)) {
      if (++pushRetry > RETRY_COUNT) {
        LOG_ERROR_SYSTEM("DataBus event queue is full for {}", typeid(Event).name());
        return;
      }
      std::this_thread::yield();
    }
  }

  void run() {
    if (running_) {
      LOG_ERROR("DataBus is already running");
      return;
    }
    LOG_DEBUG("Running DataBus");
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
        if (code != ASIO_ERR_ABORTED) {
          LOG_ERROR_SYSTEM("{}", code.message());
        }
        return;
      }
      while (!empty()) {
        (process<Events>(), ...);
      }
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

  DataBus(const DataBus &) = delete;
  DataBus(DataBus &&) = delete;
  DataBus &operator=(const DataBus &) = delete;
  DataBus &operator=(const DataBus &&) = delete;

private:
  std::atomic_bool running_{false};
  const Milliseconds rate_;

  std::tuple<UPtrLfq<Events>...> queues_;
  std::tuple<CRefHandler<Events>...> handlers_;

  CtxRunner runner_;
  SteadyTimer timer_;
};

} // namespace hft

#endif // HFT_COMMON_SYSTEMBUS_HPP