/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_COMMON_CONFIG_HPP
#define HFT_COMMON_CONFIG_HPP

#include <format>
#include <spdlog/spdlog.h>
#include <sstream>
#include <vector>

#include "logger.hpp"
#include "network_types.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft {

struct Config {
  String url;
  Port portTcpIn;
  Port portTcpOut;
  Port portUdp;

  std::vector<uint8_t> coresNetwork;
  std::vector<uint8_t> coresApp;
  Seconds coresWarmup;

  Microseconds tradeRate;
  Microseconds priceFeedRate;
  Seconds monitorRate;

  LogLevel logLevel;
  std::string logOutput;

  static Config cfg;
  static void logConfig() {
    Logger::monitorLogger->info("Url:{} TcpIn:{} TcpOut:{} Udp:{}", cfg.url, cfg.portTcpIn,
                                cfg.portTcpOut, cfg.portUdp);
    Logger::monitorLogger->info("NetworkCores:{} AppCores:{} TradeRate:{}us PriceFeedRate:{}us",
                                utils::toString(cfg.coresNetwork), utils::toString(cfg.coresApp),
                                cfg.tradeRate.count(), cfg.priceFeedRate.count());
    Logger::monitorLogger->info("LogLevel: {} LogOutput: {}", utils::toString(cfg.logLevel),
                                cfg.logOutput);
  }
};

Config Config::cfg;

} // namespace hft

#endif // HFT_SERVER_CONFIG_HPP