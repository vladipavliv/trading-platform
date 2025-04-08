/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_TRADER_CONFIG_HPP
#define HFT_TRADER_CONFIG_HPP

#include <format>
#include <spdlog/spdlog.h>
#include <sstream>
#include <vector>

#include "logging.hpp"
#include "network_types.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::trader {

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
  Microseconds tradeRate;
  Seconds monitorRate;

  // Credentials
  String name;
  String password;

  // db
  String kafkaBroker;

  // Logging
  LogLevel logLevel;
  String logOutput;

  static Config cfg;
  static void logConfig() {
    LOG_INFO_SYSTEM("Url:{} TcpUp:{} TcpDown:{} Udp:{}", cfg.url, cfg.portTcpUp, cfg.portTcpDown,
                    cfg.portUdp);
    LOG_INFO_SYSTEM("SystemCore:{} NetworkCores:{} AppCores:{} TradeRate:{}us",
                    cfg.coreSystem.value_or(0), utils::toString(cfg.coresNetwork),
                    utils::toString(cfg.coresApp), cfg.tradeRate.count());
    LOG_INFO_SYSTEM("LogLevel: {} LogOutput: {}", utils::toString(cfg.logLevel), cfg.logOutput);
    LOG_INFO_SYSTEM("Name: {} Password: {}", cfg.name, cfg.password);
  }
};

Config Config::cfg;

} // namespace hft::trader

#endif // HFT_TRADER_CONFIG_HPP