/**
 * @author Vladimir Pavliv
 * @date 2025-04-11
 */

#ifndef HFT_MONITOR_MONITORCONTROLCENTER_HPP
#define HFT_MONITOR_MONITORCONTROLCENTER_HPP

#include "adapters/kafka/kafka_adapter.hpp"
#include "bus/system_bus.hpp"
#include "client_command.hpp"
#include "config/monitor_config.hpp"
#include "console_reader.hpp"
#include "domain_types.hpp"
#include "monitor_command_parser.hpp"
#include "serialization/flat_buffers/fbs_metadata_serializer.hpp"
#include "server_command.hpp"

namespace hft::monitor {
/**
 * @brief CC
 */
class MonitorControlCenter {
  using Kafka = KafkaAdapter<serialization::FbsMetadataSerializer, MonitorCommandParser>;
  using MonitorConsoleReader = ConsoleReader<MonitorCommandParser>;

public:
  MonitorControlCenter() : consoleReader_{bus_}, kafka_{bus_} {
    // upstream subscriptions
    bus_.subscribe<OrderTimestamp>([this](CRef<OrderTimestamp> stamp) { onOrderTimestamp(stamp); });
    bus_.subscribe<MonitorCommand>([this](CRef<MonitorCommand> cmd) { onCommand(cmd); });

    // kafka setup
    kafka_.addProduceTopic<server::ServerCommand>("server-commands");
    kafka_.addProduceTopic<client::ClientCommand>("client-commands");
    kafka_.addConsumeTopic("order-timestamps");
  }

  void start() {
    greetings();

    consoleReader_.start();
    kafka_.start();

    utils::setTheadRealTime();
    if (MonitorConfig::cfg.coreSystem.has_value()) {
      utils::pinThreadToCore(MonitorConfig::cfg.coreSystem.value());
    }
    bus_.ioCtx.run();
  }

  void stop() {
    consoleReader_.stop();
    kafka_.stop();
    bus_.ioCtx.stop();
    LOG_INFO_SYSTEM("stonk");
  }

private:
  void greetings() {
    LOG_INFO_SYSTEM("Monitor go stonks");
    LOG_INFO_SYSTEM("Configuration:");
    MonitorConfig::log();
    consoleReader_.printCommands();
  }

  void onOrderTimestamp(CRef<OrderTimestamp> stamp) {
    LOG_DEBUG(utils::toString(stamp));
    //
  }

  void onCommand(CRef<MonitorCommand> cmd) { LOG_DEBUG(utils::toString(cmd)); }

private:
  SystemBus bus_;

  MonitorConsoleReader consoleReader_;
  Kafka kafka_;
};
} // namespace hft::monitor

#endif // HFT_MONITOR_MONITORCONTROLCENTER_HPP