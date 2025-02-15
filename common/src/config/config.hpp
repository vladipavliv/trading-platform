/**
 * @file config.hpp
 * @brief
 *
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
  uint8_t threadsIo;
  uint8_t threadsApp;
  bool threadsPin;

  static Config cfg;

  static void logConfig(const Config &config) {
    spdlog::debug("url: {} tcp port in: {} tcp port out: {} udp port: {} threads io: {} threads "
                  "app: {} threads pin: ",
                  config.url, config.portTcpIn, config.portTcpOut, config.portUdp, config.threadsIo,
                  config.threadsApp, config.threadsPin);
  }
};

Config Config::cfg;

} // namespace hft

#endif // HFT_SERVER_CONFIG_HPP