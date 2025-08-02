/**
 * @author Vladimir Pavliv
 * @date 2025-04-11
 */

#ifndef HFT_MONITOR_MONITORCONTROLCENTER_HPP
#define HFT_MONITOR_MONITORCONTROLCENTER_HPP

#include "adapters/kafka/kafka_adapter.hpp"
#include "bus/system_bus.hpp"
#include "client_command.hpp"
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
  using Kafka = KafkaAdapter<serialization::ProtoMetadataSerializer, MonitorCommandParser>;
  using MonitorConsoleReader = ConsoleReader<MonitorCommandParser>;

public:
  MonitorControlCenter() : consoleReader_{bus_}, kafka_{bus_}, tracker_{bus_} {
    bus_.subscribe(MonitorCommand::Shutdown, [this] { stop(); });

    kafka_.addProduceTopic<server::ServerCommand>(
        Config::get<String>("kafka.kafka_server_cmd_topic"));
    kafka_.addProduceTopic<client::ClientCommand>(
        Config::get<String>("kafka.kafka_client_cmd_topic"));
    kafka_.addConsumeTopic(Config::get<String>("kafka.kafka_timestamps_topic"));
    kafka_.addConsumeTopic(Config::get<String>("kafka.kafka_metrics_topic"));
  }

  void start() {
    greetings();
    kafka_.start();
    bus_.run();
  }

  void stop() {
    kafka_.stop();
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

  MonitorConsoleReader consoleReader_;
  Kafka kafka_;
  LatencyTracker tracker_;
};
} // namespace hft::monitor

#endif // HFT_MONITOR_MONITORCONTROLCENTER_HPP
