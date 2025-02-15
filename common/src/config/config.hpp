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

struct NetworkConfig {
  String url;
  Port portTcpIn;
  Port portTcpOut;
  Port portUdp;
};

struct Config {
  NetworkConfig server;
  NetworkConfig trader;

  static Config cfg;

  static void logConfig(const Config &config) {
    logServerConfig(config.server);
    logTraderConfig(config.trader);
  }
  static void logServerConfig(const NetworkConfig &config) {
    spdlog::debug(
        "Server configuration: url: {}, ingress tcp port: {}, egress tcp port: {}, udp port: {}",
        config.url, config.portTcpIn, config.portTcpOut, config.portUdp);
  }

  static void logTraderConfig(const NetworkConfig &config) {
    spdlog::debug(
        "Trader configuration: url: {}, ingress tcp port: {}, egress tcp port: {}, udp port: {}",
        config.url, config.portTcpIn, config.portTcpOut, config.portUdp);
  }
};

Config Config::cfg;

} // namespace hft

#endif // HFT_SERVER_CONFIG_HPP