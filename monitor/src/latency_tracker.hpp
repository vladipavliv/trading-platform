/**
 * @author Vladimir Pavliv
 * @date 2025-04-11
 */

#ifndef HFT_MONITOR_LATENCYTRACKER_HPP
#define HFT_MONITOR_LATENCYTRACKER_HPP

#include "bus/system_bus.hpp"
#include "config/monitor_config.hpp"
#include "domain_types.hpp"
#include "execution.hpp"
#include "primitive_types.hpp"
#include "rtt_tracker.hpp"
#include "traits.hpp"

namespace hft::monitor {
/**
 * @brief
 */
class LatencyTracker {
  using Tracker = RttTracker<1000, 10000>;

  struct LatencyStats {
    AtomicUInt64 sum{0};
    AtomicUInt64 size{0};
  };

public:
  explicit LatencyTracker(MonitorBus &bus)
      : bus_{bus}, statsTimer_{bus_.systemIoCtx()},
        monitorRate_{
            Milliseconds(Config::get_optional<size_t>("rates.monitor_rate_ms").value_or(1000))} {
    bus_.subscribe<Order>([this](CRef<Order> msg) { onOrder(msg); });
    scheduleStatsTimer();
  }

private:
  void onOrder(CRef<Order> msg) {}

  void scheduleStatsTimer() {
    using namespace utils;
    statsTimer_.expires_after(monitorRate_);
    statsTimer_.async_wait([this](BoostErrorCode code) {
      if (code) {
        if (code != ERR_ABORTED) {
          LOG_ERROR_SYSTEM("{}", code.message());
        }
        return;
      }
      static uint64_t lastCounter{0};
      auto counter = counter_.load(std::memory_order_relaxed);
      if (counter != lastCounter) {
        LOG_INFO_SYSTEM("{}", Tracker::getStatsString());
      }
      lastCounter = counter;
      scheduleStatsTimer();
    });
  }

private:
  MonitorBus &bus_;

  const Milliseconds monitorRate_;
  SteadyTimer statsTimer_;

  AtomicUInt64 counter_;
};
} // namespace hft::monitor

#endif // HFT_MONITOR_LATENCYTRACKER_HPP
