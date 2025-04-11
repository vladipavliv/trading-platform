/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_SERVER_CONFIG_HPP
#define HFT_SERVER_CONFIG_HPP

#include <format>
#include <sstream>
#include <vector>

#include "logging.hpp"
#include "network_types.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::server {

struct Config {
  // Network
  String url;
  Port portTcpUp;
  Port portTcpDown;
  Port portUdp;

  // Cores
  Opt<CoreId> coreSystem;
  std::vector<CoreId> coresNetwork;
  std::vector<CoreId> coresApp;

  // Rates
  Microseconds priceFeedRate;
  Seconds monitorRate;

  // kafka
  String kafkaBroker;
  String kafkaConsumerGroup;
  Milliseconds kafkaPollRate;

  // Logging
  LogLevel logLevel;
  String logOutput;

  static Config cfg;
  static void logConfig() {
    LOG_INFO_SYSTEM("Url:{} TcpUp:{} TcpDown:{} Udp:{}", cfg.url, cfg.portTcpUp, cfg.portTcpDown,
                    cfg.portUdp);
    LOG_INFO_SYSTEM("SystemCore:{} NetworkCores:{} AppCores:{} PriceFeedRate:{}us",
                    cfg.coreSystem.value_or(0), utils::toString(cfg.coresNetwork),
                    utils::toString(cfg.coresApp), cfg.priceFeedRate.count());
    LOG_INFO_SYSTEM("Kafka broker:{} consumer group:{} poll rate:{}", cfg.kafkaBroker,
                    cfg.kafkaConsumerGroup, cfg.kafkaPollRate.count());
    LOG_INFO_SYSTEM("LogLevel: {} LogOutput: {}", utils::toString(cfg.logLevel), cfg.logOutput);
  }
};

Config Config::cfg;

} // namespace hft::server

#endif // HFT_SERVER_CONFIG_HPP