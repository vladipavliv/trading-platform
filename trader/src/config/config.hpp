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
  Port portTcpIn;
  Port portTcpOut;
  Port portUdp;

  // Cores
  uint8_t coreSystem;
  std::vector<uint8_t> coresNetwork;
  std::vector<uint8_t> coresApp;

  // Rates
  Microseconds tradeRate;
  Seconds monitorRate;

  // Credentials
  String name;
  String password;

  // Logging
  LogLevel logLevel;
  String logOutput;

  static Config cfg;
  static void logConfig() {
    LOG_INFO_SYSTEM("Url:{} TcpIn:{} TcpOut:{} Udp:{}", cfg.url, cfg.portTcpIn, cfg.portTcpOut,
                    cfg.portUdp);
    LOG_INFO_SYSTEM("SystemCore:{} NetworkCores:{} AppCores:{} TradeRate:{}us", cfg.coreSystem,
                    utils::toString(cfg.coresNetwork), utils::toString(cfg.coresApp),
                    cfg.tradeRate.count());
    LOG_INFO_SYSTEM("LogLevel: {} LogOutput: {}", utils::toString(cfg.logLevel), cfg.logOutput);
    LOG_INFO_SYSTEM("Name: {} Password: {}", cfg.name, cfg.password);
  }
};

Config Config::cfg;

} // namespace hft::trader

#endif // HFT_TRADER_CONFIG_HPP