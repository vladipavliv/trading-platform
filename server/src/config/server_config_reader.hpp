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

#include "boost_types.hpp"
#include "server_config.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft::server {

struct ServerConfigReader {
  static void readConfig(CRef<String> fileName) {
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(fileName, pt);

    // Network
    ServerConfig::cfg.url = pt.get<String>("network.url");
    ServerConfig::cfg.portTcpUp = pt.get<int>("network.port_tcp_up");
    ServerConfig::cfg.portTcpDown = pt.get<int>("network.port_tcp_down");
    ServerConfig::cfg.portUdp = pt.get<int>("network.port_udp");

    // Cores
    if (const auto core = pt.get_optional<int>("cpu.core_system")) {
      ServerConfig::cfg.coreSystem = *core;
    }
    if (const auto cores = pt.get_optional<String>("cpu.cores_network")) {
      ServerConfig::cfg.coresNetwork = parseCores(*cores);
    }
    if (const auto cores = pt.get_optional<String>("cpu.cores_app")) {
      ServerConfig::cfg.coresApp = parseCores(*cores);
    }

    // Rates
    ServerConfig::cfg.priceFeedRate = Microseconds(pt.get<int>("rates.price_feed_rate"));
    ServerConfig::cfg.monitorRate = Seconds(pt.get<int>("rates.monitor_rate"));

    // kafka
    ServerConfig::cfg.kafkaBroker = pt.get<String>("kafka.kafka_broker");
    ServerConfig::cfg.kafkaConsumerGroup = pt.get<String>("kafka.kafka_consumer_group");
    ServerConfig::cfg.kafkaPollRate = Milliseconds(pt.get<int>("kafka.kafka_poll_rate"));

    // Logging
    ServerConfig::cfg.logLevel = utils::fromString<LogLevel>(pt.get<std::string>("log.level"));
    ServerConfig::cfg.logOutput = pt.get<String>("log.output");

    verifyConfig(ServerConfig::cfg);
  }

  static void verifyConfig(CRef<ServerConfig> cfg) {
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
    if (cfg.kafkaBroker.empty() || cfg.kafkaConsumerGroup.empty()) {
      throw std::runtime_error("Invalid kafka configuration");
    }
    if (cfg.logOutput.empty()) {
      throw std::runtime_error("Invalid log file");
    }
  }

  static ByteBuffer parseCores(CRef<String> input) {
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