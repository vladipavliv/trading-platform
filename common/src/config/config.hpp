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

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "config.hpp"
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
  uint8_t networkThreads;

  static const Config &config() {
    static std::optional<Config> staticConfig;
    if (staticConfig.has_value()) {
      return staticConfig.value();
    }
    boost::property_tree::ptree pt;
    std::string filename{"config.ini"};

    boost::property_tree::read_ini(filename, pt);

    Config cfg;
    // Server config
    cfg.server.url = pt.get<std::string>("server.url");
    cfg.server.portTcpIn = pt.get<int>("server.port_tcp_in");
    cfg.server.portTcpOut = pt.get<int>("server.port_tcp_out");
    cfg.server.portUdp = pt.get<int>("server.port_udp");

    // Trader config
    cfg.trader.url = pt.get<std::string>("trader.url");
    cfg.trader.portTcpIn = pt.get<int>("trader.port_tcp_in");
    cfg.trader.portTcpOut = pt.get<int>("trader.port_tcp_out");
    cfg.trader.portUdp = pt.get<int>("trader.port_udp");

    // cpu
    cfg.networkThreads = pt.get<int>("cpu.network_threads");

    staticConfig.emplace(cfg);
    return *staticConfig;
  }
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

} // namespace hft

#endif // HFT_SERVER_CONFIG_HPP