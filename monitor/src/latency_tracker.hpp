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
#include "traits.hpp"
#include "utils/rtt_tracker.hpp"

namespace hft::monitor {
/**
 * @brief
 */
class LatencyTracker {
  using Tracker = RttTracker<RTT_RANGES>;

public:
  explicit LatencyTracker(Context &ctx)
      : ctx_{ctx}, statsTimer_{ctx_.bus.systemIoCtx()},
        monitorRate_{Milliseconds(
            ctx.config.data.get_optional<size_t>("rates.monitor_rate_ms").value_or(1000))} {
    ctx_.bus.subscribe<TelemetryMsg>(
        CRefHandler<TelemetryMsg>::template bind<LatencyTracker, &LatencyTracker::post>(this));

    scheduleStatsTimer();
  }

private:
  void post(CRef<TelemetryMsg> msg) {
    switch (msg.type) {
    case TelemetryType::Startup:
      break;
    case TelemetryType::OrderLatency: {
      auto rtt = (msg.data.order.notified - msg.data.order.created) * ctx_.config.nsPerCycle;
      Tracker::logRtt(rtt);
    } break;
    case TelemetryType::Runtime:
      break;
    case TelemetryType::Profiling:
      break;
    case TelemetryType::Log:
      break;
    default:
      break;
    }
  }

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
      const auto stats = Tracker::getStats();
      const auto volume = stats.volume();
      if (volume != 0) {
        const auto rps = (volume * 1000) / monitorRate_.count();
        LOG_INFO_SYSTEM("Rps: {} Rtt: {}", thousandify(rps), Tracker::toString(stats));
        Tracker::reset();
      }
      scheduleStatsTimer();
    });
  }

private:
  Context &ctx_;

  const Milliseconds monitorRate_;
  SteadyTimer statsTimer_;
};
} // namespace hft::monitor

#endif // HFT_MONITOR_LATENCYTRACKER_HPP
