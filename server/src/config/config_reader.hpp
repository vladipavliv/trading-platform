/**
 * @author Vladimir Pavliv
 * @date 2025-03-30
 */

#ifndef HFT_SERVER_CONFIGREADER_HPP
#define HFT_SERVER_CONFIGREADER_HPP

#include <sstream>
#include <vector>

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "config.hpp"
#include "network_types.hpp"
#include "types.hpp"
#include "utils/template_utils.hpp"

namespace hft::server {

struct ConfigReader {
  static void readConfig(CRef<String> fileName) {
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(fileName, pt);

    // Network
    Config::cfg.url = pt.get<String>("network.url");
    Config::cfg.portTcpUp = pt.get<int>("network.port_tcp_up");
    Config::cfg.portTcpDown = pt.get<int>("network.port_tcp_down");
    Config::cfg.portUdp = pt.get<int>("network.port_udp");

    // Cores
    if (auto v = pt.get_optional<int>("cpu.core_system")) {
      Config::cfg.coreSystem = *v;
    }
    if (auto v = pt.get_optional<String>("cpu.cores_network")) {
      Config::cfg.coresNetwork = parseCores(*v);
    }
    if (auto v = pt.get_optional<String>("cpu.cores_app")) {
      Config::cfg.coresApp = parseCores(*v);
    }

    // Rates
    Config::cfg.priceFeedRate = Microseconds(pt.get<int>("rates.price_feed_rate"));
    Config::cfg.monitorRate = Seconds(pt.get<int>("rates.monitor_rate"));

    // db
    Config::cfg.kafkaBroker = pt.get<String>("db.kafka_broker");

    // Logging
    Config::cfg.logLevel = utils::fromString<LogLevel>(pt.get<std::string>("log.level"));
    Config::cfg.logOutput = pt.get<String>("log.output");

    verifyConfig(Config::cfg);
  }

  static void verifyConfig(CRef<Config> cfg) {
    if (cfg.url.empty() || cfg.portTcpUp == 0 || cfg.portTcpDown == 0 || cfg.portUdp == 0) {
      throw std::runtime_error("Invalid network configuration");
    }
    if (utils::hasIntersection(cfg.coresApp, cfg.coresNetwork)) {
      throw std::runtime_error("Invalid cores configuration");
    }
    if (std::find(cfg.coresApp.begin(), cfg.coresApp.end(), cfg.coreSystem) != cfg.coresApp.end()) {
      throw std::runtime_error("Invalid cores configuration");
    }
    if (std::find(cfg.coresNetwork.begin(), cfg.coresNetwork.end(), cfg.coreSystem) !=
        cfg.coresNetwork.end()) {
      throw std::runtime_error("Invalid cores configuration");
    }
    if (cfg.logOutput.empty()) {
      throw std::runtime_error("Invalid log file");
    }
  }

  static ByteBuffer parseCores(CRefString input) {
    ByteBuffer result;
    std::stringstream ss(input);
    std::string token;

    while (std::getline(ss, token, ',')) {
      result.push_back(static_cast<uint8_t>(std::stoi(token)));
    }
    return result;
  }
};

} // namespace hft::server

#endif // HFT_SERVER_CONFIGREADER_HPP