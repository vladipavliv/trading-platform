/**
 * @file config.hpp
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_COMMON_CONFIGREADER_HPP
#define HFT_COMMON_CONFIGREADER_HPP

#include <spdlog/spdlog.h>
#include <sstream>
#include <vector>

#ifdef CXX_HAS_RTTI
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#endif

#include "config.hpp"
#include "network_types.hpp"
#include "types.hpp"

namespace hft {

struct ConfigReader {

#ifdef CXX_HAS_RTTI
  static void readConfig() {
    boost::property_tree::ptree pt;
    std::string filename{"config.ini"};

    boost::property_tree::read_ini(filename, pt);

    Config config;
    // Server config
    config.server.url = pt.get<std::string>("server.url");
    config.server.portTcpIn = pt.get<int>("server.port_tcp_in");
    config.server.portTcpOut = pt.get<int>("server.port_tcp_out");
    config.server.portUdp = pt.get<int>("server.port_udp");

    // Trader config
    config.trader.url = pt.get<std::string>("trader.url");
    config.trader.portTcpIn = pt.get<int>("trader.port_tcp_in");
    config.trader.portTcpOut = pt.get<int>("trader.port_tcp_out");
    config.trader.portUdp = pt.get<int>("trader.port_udp");

    Config::cfg = config;
  }
#else
  static void readConfig() {
    Config::cfg =
        Config{NetworkConfig{"127.0.0.1", 8080, 8081}, NetworkConfig{"127.0.0.1", 8081, 8080}};
  }
#endif
};

} // namespace hft

#endif // HFT_COMMON_CONFIGREADER_HPP