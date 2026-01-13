/**
 * @author Vladimir Pavliv
 * @date 2025-04-11
 */

#ifndef HFT_MONITOR_CONTROLCENTER_HPP
#define HFT_MONITOR_CONTROLCENTER_HPP

#include "bus/system_bus.hpp"
#include "commands/command.hpp"
#include "commands/command_parser.hpp"
#include "config/monitor_config.hpp"
#include "console_reader.hpp"
#include "domain_types.hpp"
#include "latency_tracker.hpp"
#include "server/src/commands/command.hpp"
#include "traits.hpp"

namespace hft::monitor {
/**
 * @brief CC
 */
class MonitorControlCenter {
public:
  MonitorControlCenter() : consoleReader_{bus_.systemBus}, telemetry_{bus_, false}, tracker_{bus_} {
    bus_.subscribe(Command::Shutdown, [this] { stop(); });
  }

  void start() {
    greetings();
    bus_.run();
  }

  void stop() {
    telemetry_.close();
    bus_.stop();
    LOG_INFO_SYSTEM("stonk");
  }

private:
  void greetings() {
    LOG_INFO_SYSTEM("Monitor go stonks");
    LOG_INFO_SYSTEM("Configuration:");
    MonitorConfig::log();
    consoleReader_.printCommands();
  }

private:
  MonitorBus bus_;

  MonitorConsoleReader consoleReader_;
  MonitorTelemetry telemetry_;
  LatencyTracker tracker_;
};
} // namespace hft::monitor

#endif // HFT_MONITOR_CONTROLCENTER_HPP
