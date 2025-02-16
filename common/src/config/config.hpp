/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_COMMON_CONFIG_HPP
#define HFT_COMMON_CONFIG_HPP

#include <spdlog/spdlog.h>
#include <sstream>
#include <vector>

#include "network_types.hpp"
#include "types.hpp"

namespace hft {

struct Config {
  String url;
  Port portTcpIn;
  Port portTcpOut;
  Port portUdp;
  std::vector<uint8_t> coresIo;
  std::vector<uint8_t> coresApp;

  static Config cfg;

  static void logConfig(const Config &config) {
    spdlog::debug("url: {} tcp port in: {} tcp port out: {} udp port: {} cores: {} core ids: {}",
                  config.url, config.portTcpIn, config.portTcpOut, config.portUdp, CORES, CORE_IDS);
  }
};

Config Config::cfg;

} // namespace hft

#endif // HFT_SERVER_CONFIG_HPP