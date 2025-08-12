/**
 * @author Vladimir Pavliv
 * @date 2025-04-11
 */

#ifndef HFT_MONITOR_LATENCYTRACKER_HPP
#define HFT_MONITOR_LATENCYTRACKER_HPP

#include "bus/system_bus.hpp"
#include "config/monitor_config.hpp"
#include "domain_types.hpp"
#include "monitor_types.hpp"
#include "types.hpp"

namespace hft::monitor {
/**
 * @brief Tracks the latencies
 * @todo Use HdrHistogram. This tracker is single-threaded, so no
 * histogram-merge complications here for multi-threaded tracking
 */
class LatencyTracker {
  struct Latency {
    uint64_t sum{0};
    uint64_t size{0};
  };

public:
  explicit LatencyTracker(MonitorBus &bus)
      : bus_{bus}, statsTimer_{bus_.systemIoCtx()},
        monitorRate_{Config::get_optional<size_t>("rates.monitor_rate_ms").value_or(1000)} {
    bus_.subscribe<OrderTimestamp>([this](CRef<OrderTimestamp> msg) { onOrderTimestamp(msg); });
    bus_.subscribe<RuntimeMetrics>([this](CRef<RuntimeMetrics> msg) { onRuntimeMetrics(msg); });
    scheduleStatsTimer();
  }

private:
  void onOrderTimestamp(CRef<OrderTimestamp> msg) {
    LOG_DEBUG("onOrderTimestamp {}", utils::toString(msg));
    rtt_.sum += msg.notified - msg.created;
    rtt_.size++;
  }

  void onRuntimeMetrics(CRef<RuntimeMetrics> msg) {
    LOG_DEBUG("onRuntimeMetrics {}", utils::toString(msg));
    rps_ = msg.rps;
  }

  void scheduleStatsTimer() {
    using namespace utils;
    statsTimer_.expires_after(monitorRate_);
    statsTimer_.async_wait([this](BoostErrorCode code) {
      if (code) {
        if (code != ASIO_ERR_ABORTED) {
          LOG_ERROR_SYSTEM("{}", code.message());
        }
        return;
      }
      static uint64_t lastSize{0};
      if (rtt_.size != 0 && rtt_.size != lastSize) {
        LOG_INFO_SYSTEM("Orders: {} | Rtt: {}us | Rps: {}", rtt_.size, rtt_.sum / rtt_.size, rps_);
      }
      lastSize = rtt_.size;
      scheduleStatsTimer();
    });
  }

private:
  MonitorBus &bus_;

  const Milliseconds monitorRate_;
  SteadyTimer statsTimer_;

  Latency rtt_;
  uint64_t rps_{0};
};
} // namespace hft::monitor

#endif // HFT_MONITOR_LATENCYTRACKER_HPP
