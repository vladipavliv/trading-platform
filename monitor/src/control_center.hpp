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
public:
  MonitorControlCenter()
      : reactor_{ErrorBus{bus_.systemBus}}, consoleReader_{bus_.systemBus}, telemetry_{bus_, false},
        tracker_{bus_} {
    bus_.subscribe(
        Command::Shutdown,
        Callback::template bind<MonitorControlCenter, &MonitorControlCenter::stop>(this));
  }

  void start() {
    greetings();
    telemetry_.start();
    reactor_.run();
    bus_.run();
  }

  void stop() {
    reactor_.stop();
    telemetry_.close();
    bus_.stop();
    LOG_INFO_SYSTEM("stonk");
  }

private:
  void greetings() {
    LOG_INFO_SYSTEM("Monitor go stonks");
    LOG_INFO_SYSTEM("Configuration:");
    MonitorConfig::cfg().log();
    consoleReader_.printCommands();
  }

private:
  MonitorBus bus_;

  ShmReactor reactor_;

  MonitorConsoleReader consoleReader_;
  MonitorTelemetry telemetry_;
  LatencyTracker tracker_;
};
} // namespace hft::monitor

#endif // HFT_MONITOR_CONTROLCENTER_HPP
