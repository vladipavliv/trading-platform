/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_TRADER_CONFIG_HPP
#define HFT_TRADER_CONFIG_HPP

#include <vector>

#include "logging.hpp"
#include "network_types.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::trader {

struct TraderConfig {
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
  Microseconds tradeRate;
  Seconds monitorRate;

  // Credentials
  String name;
  String password;

  // kafka
  String kafkaBroker;
  String kafkaConsumerGroup;
  Milliseconds kafkaPollRate;

  // Logging
  LogLevel logLevel;
  String logOutput;

  static TraderConfig cfg;

  static void logConfig() {
    LOG_INFO_SYSTEM("Url:{} TcpUp:{} TcpDown:{} Udp:{}", cfg.url, cfg.portTcpUp, cfg.portTcpDown,
                    cfg.portUdp);
    LOG_INFO_SYSTEM("SystemCore:{} NetworkCores:{} AppCores:{} TradeRate:{}us",
                    cfg.coreSystem.value_or(0), utils::toString(cfg.coresNetwork),
                    utils::toString(cfg.coresApp), cfg.tradeRate.count());
    LOG_INFO_SYSTEM("Kafka broker:{} consumer group:{} poll rate:{}", cfg.kafkaBroker,
                    cfg.kafkaConsumerGroup, cfg.kafkaPollRate.count());
    LOG_INFO_SYSTEM("LogLevel: {} LogOutput: {}", utils::toString(cfg.logLevel), cfg.logOutput);
    LOG_INFO_SYSTEM("Name: {} Password: {}", cfg.name, cfg.password);
  }
};

TraderConfig TraderConfig::cfg;

} // namespace hft::trader

#endif // HFT_TRADER_CONFIG_HPP