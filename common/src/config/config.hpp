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
#include "utils/string_utils.hpp"

namespace hft {

struct Config {
  String url;
  Port portTcpIn;
  Port portTcpOut;
  Port portUdp;
  std::vector<uint8_t> coresIo;
  std::vector<uint8_t> coresApp;

  static Config cfg;
  static void log() {
    spdlog::debug(
        "url: {} tcp port in: {} tcp port out: {} udp port: {} io cores: {} app cores: {}", cfg.url,
        cfg.portTcpIn, cfg.portTcpOut, cfg.portUdp, utils::toString(cfg.coresIo),
        utils::toString(cfg.coresApp));
  }
};

Config Config::cfg;

} // namespace hft

#endif // HFT_SERVER_CONFIG_HPP