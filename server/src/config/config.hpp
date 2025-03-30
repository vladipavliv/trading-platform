/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_SERVER_CONFIG_HPP
#define HFT_SERVER_CONFIG_HPP

#include <format>
#include <spdlog/spdlog.h>
#include <sstream>
#include <vector>

#include "logger.hpp"
#include "network_types.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::server {

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
  Microseconds priceFeedRate;
  Seconds monitorRate;

  // Logging
  LogLevel logLevel;
  String logOutput;

  static Config cfg;
  static void logConfig() {
    Logger::monitorLogger->info("Url:{} TcpIn:{} TcpOut:{} Udp:{}", cfg.url, cfg.portTcpIn,
                                cfg.portTcpOut, cfg.portUdp);
    Logger::monitorLogger->info("SystemCore:{} NetworkCores:{} AppCores:{} PriceFeedRate:{}us",
                                cfg.coreSystem, utils::toString(cfg.coresNetwork),
                                utils::toString(cfg.coresApp), cfg.priceFeedRate.count());
    Logger::monitorLogger->info("LogLevel: {} LogOutput: {}", utils::toString(cfg.logLevel),
                                cfg.logOutput);
  }
};

Config Config::cfg;

} // namespace hft::server

#endif // HFT_SERVER_CONFIG_HPP