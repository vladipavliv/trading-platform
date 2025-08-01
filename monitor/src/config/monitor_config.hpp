/**
 * @author Vladimir Pavliv
 * @date 2025-04-11
 */

#ifndef HFT_MONITOR_CONFIG_HPP
#define HFT_MONITOR_CONFIG_HPP

#include "boost_types.hpp"
#include "config/config.hpp"
#include "logging.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::monitor {

struct MonitorConfig {
  // cores
  Optional<CoreId> coreSystem;

  // kafka
  String kafkaBroker;
  String kafkaConsumerGroup;
  Milliseconds kafkaPollRate;

  // Logging
  String logOutput;

  static MonitorConfig cfg;

  static void load(CRef<String> fileName) {
    Config::load(fileName);

    // cores
    if (auto core = Config::get_optional<size_t>("cpu.core_system")) {
      MonitorConfig::cfg.coreSystem = *core;
    }

    // kafka
    MonitorConfig::cfg.kafkaBroker = Config::get<String>("kafka.kafka_broker");
    MonitorConfig::cfg.kafkaConsumerGroup = Config::get<String>("kafka.kafka_consumer_group");
    MonitorConfig::cfg.kafkaPollRate =
        Milliseconds(Config::get<size_t>("kafka.kafka_poll_rate_us"));

    // Logging
    MonitorConfig::cfg.logOutput = Config::get<String>("log.output");

    verify();
  }

  static void verify() {
    if (cfg.kafkaBroker.empty() || cfg.kafkaConsumerGroup.empty()) {
      throw std::runtime_error("Invalid kafka configuration");
    }
  }

  static void log() {
    LOG_INFO_SYSTEM("Kafka broker:{} consumer group:{} poll rate:{}", cfg.kafkaBroker,
                    cfg.kafkaConsumerGroup, cfg.kafkaPollRate.count());
    LOG_INFO_SYSTEM("LogOutput: {}", cfg.logOutput);
  }
};

inline MonitorConfig MonitorConfig::cfg;

} // namespace hft::monitor

#endif // HFT_MONITOR_CONFIG_HPP