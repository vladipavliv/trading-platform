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
  std::vector<uint8_t> coresIo;
  std::vector<uint8_t> coresApp;
  size_t tradeRateUs;
  size_t priceFeedRateUs;
  uint16_t monitorRateS;
  bool monitorStats{false};

  static Config cfg;
  static void logConfig() {
    Logger::monitorLogger->info("Url:{} TcpIn:{} TcpOut:{} Udp:{}", cfg.url, cfg.portTcpIn,
                                cfg.portTcpOut, cfg.portUdp);
    Logger::monitorLogger->info("IoCoreIDs:{} AppCoreIDs:{}", utils::toString(cfg.coresIo),
                                utils::toString(cfg.coresApp), cfg.tradeRateUs,
                                cfg.priceFeedRateUs);
    Logger::monitorLogger->info("Trade rate:{} Pricefeed rate:{} monitor:{}s",
                                utils::getScaleUs(cfg.tradeRateUs),
                                utils::getScaleUs(cfg.priceFeedRateUs), cfg.monitorRateS);
  }
};

Config Config::cfg;

} // namespace hft

#endif // HFT_SERVER_CONFIG_HPP