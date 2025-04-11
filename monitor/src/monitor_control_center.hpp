/**
 * @author Vladimir Pavliv
 * @date 2025-04-11
 */

#ifndef HFT_MONITOR_MONITORCONTROLCENTER_HPP
#define HFT_MONITOR_MONITORCONTROLCENTER_HPP

#include "adapters/kafka/kafka_adapter.hpp"
#include "bus/system_bus.hpp"
#include "monitor_command_parser.hpp"
#include "serialization/flat_buffers/metadata_serializer.hpp"

namespace hft::monitor {
/**
 * @brief CC
 */
class MonitorControlCenter {
  using Kafka = KafkaAdapter<serialization::fbs::MetadataSerializer, MonitorCommandParser>;

public:
  MonitorControlCenter() : kafka_{bus_, kafkaCfg()} {}

  void start() {}

private:
  KafkaConfig kafkaCfg() const {
    return KafkaConfig{Config::cfg.kafkaBroker, Config::cfg.kafkaConsumerGroup,
                       Config::cfg.kafkaPollRate};
  }

private:
  SystemBus bus_;
  Kafka kafka_;
};
} // namespace hft::monitor

#endif // HFT_MONITOR_MONITORCONTROLCENTER_HPP