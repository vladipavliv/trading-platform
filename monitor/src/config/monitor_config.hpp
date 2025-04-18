/**
 * @author Vladimir Pavliv
 * @date 2025-04-11
 */

#ifndef HFT_MONITOR_CONFIG_HPP
#define HFT_MONITOR_CONFIG_HPP

#include "boost_types.hpp"
#include "logging.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::monitor {

struct MonitorConfig {
  // cores
  Opt<CoreId> coreSystem;

  // kafka
  String kafkaBroker;
  String kafkaConsumerGroup;
  Milliseconds kafkaPollRate;

  // Logging
  LogLevel logLevel;
  String logOutput;

  static MonitorConfig cfg;

  static void logConfig() {
    LOG_INFO_SYSTEM("Kafka broker:{} consumer group:{} poll rate:{}", cfg.kafkaBroker,
                    cfg.kafkaConsumerGroup, cfg.kafkaPollRate.count());
    LOG_INFO_SYSTEM("LogLevel: {} LogOutput: {}", utils::toString(cfg.logLevel), cfg.logOutput);
  }
};

MonitorConfig MonitorConfig::cfg;

} // namespace hft::monitor

#endif // HFT_MONITOR_CONFIG_HPP