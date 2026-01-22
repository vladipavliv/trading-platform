/**
 * @author Vladimir Pavliv
 * @date 2025-04-11
 */

#ifndef HFT_MONITOR_CONTROLCENTER_HPP
#define HFT_MONITOR_CONTROLCENTER_HPP

#include "bus/bus_hub.hpp"
#include "bus/system_bus.hpp"
#include "commands/command.hpp"
#include "commands/command_parser.hpp"
#include "config/monitor_config.hpp"
#include "domain_types.hpp"
#include "latency_tracker.hpp"
#include "server/src/commands/command.hpp"
#include "traits.hpp"
#include "utils/console_reader.hpp"

namespace hft::monitor {
/**
 * @brief CC
 */
class MonitorControlCenter {
  using SelfT = MonitorControlCenter;

public:
  explicit MonitorControlCenter(MonitorConfig &&cfg)
      : config_{std::move(cfg)}, bus_{config_.data}, ctx_{bus_, config_, stopSrc_.get_token()},
        reactor_{config_.data, ctx_.stopToken, ErrorBus{bus_.systemBus}},
        consoleReader_{bus_.systemBus}, telemetry_{bus_, config_.data, false}, tracker_{ctx_} {
    bus_.subscribe(Command::Shutdown, Callback::bind<SelfT, &SelfT::stop>(this));
  }

  void start() {
    greetings();
    try {
      telemetry_.start();
      reactor_.run();
      bus_.run();
    } catch (const std::exception &e) {
      LOG_ERROR_SYSTEM("Exception in CC::run {}", e.what());
    }
  }

  void stop() {
    LOG_DEBUG_SYSTEM("Monitor::CC stop");
    try {
      stopSrc_.request_stop();

      reactor_.stop();
      telemetry_.close();
      bus_.stop();

      LOG_INFO_SYSTEM("stonk");
    } catch (const std::exception &e) {
      LOG_ERROR_SYSTEM("Exception in CC::stop {}", e.what());
    }
  }

private:
  void greetings() {
    LOG_INFO_SYSTEM("Monitor go stonks");
    LOG_INFO_SYSTEM("Configuration:");
    consoleReader_.printCommands();
  }

private:
  const MonitorConfig config_;
  std::stop_source stopSrc_;

  MonitorBus bus_;

  Context ctx_;
  ShmReactor reactor_;

  MonitorConsoleReader consoleReader_;
  MonitorTelemetry telemetry_;
  LatencyTracker tracker_;
};
} // namespace hft::monitor

#endif // HFT_MONITOR_CONTROLCENTER_HPP
