/**
 * @author Vladimir Pavliv
 * @date 2025-04-11
 */

#ifndef HFT_MONITOR_MONITORCONTROLCENTER_HPP
#define HFT_MONITOR_MONITORCONTROLCENTER_HPP

#include "adapters/adapters.hpp"
#include "bus/system_bus.hpp"
#include "commands/client_command.hpp"
#include "commands/server_command.hpp"
#include "config/monitor_config.hpp"
#include "console_reader.hpp"
#include "domain_types.hpp"
#include "latency_tracker.hpp"
#include "monitor_command.hpp"
#include "monitor_command_parser.hpp"
#include "serialization/protobuf/proto_metadata_serializer.hpp"

namespace hft::monitor {
/**
 * @brief CC
 */
class MonitorControlCenter {
public:
  using MonitorConsoleReader = ConsoleReader<MonitorCommandParser>;
  using StreamAdapter =
      adapters::MessageQueueAdapter<SystemBus, serialization::ProtoMetadataSerializer,
                                    MonitorCommandParser>;

  MonitorControlCenter() : consoleReader_{bus_}, streamAdapter_{bus_}, tracker_{bus_} {
    using namespace server;
    using namespace client;

    bus_.subscribe(MonitorCommand::Shutdown, [this] { stop(); });
  }

  void start() {
    greetings();

    streamAdapter_.start();
    streamAdapter_.bindProduceTopic<client::ClientCommand>("client-commands");
    streamAdapter_.bindProduceTopic<server::ServerCommand>("server-commands");

    bus_.run();
  }

  void stop() {
    streamAdapter_.stop();
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
  SystemBus bus_;

  StreamAdapter streamAdapter_;
  MonitorConsoleReader consoleReader_;
  LatencyTracker tracker_;
};
} // namespace hft::monitor

#endif // HFT_MONITOR_MONITORCONTROLCENTER_HPP
