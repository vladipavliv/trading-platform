/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_CLIENT_CONFIGREADER_HPP
#define HFT_CLIENT_CONFIGREADER_HPP

#include <sstream>
#include <vector>

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "boost_types.hpp"
#include "client_config.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft::client {

struct ClientConfigReader {
  static void readConfig(CRef<String> fileName) {
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(fileName, pt);

    // Network config
    ClientConfig::cfg.url = pt.get<String>("network.url");
    ClientConfig::cfg.portTcpUp = pt.get<int>("network.port_tcp_up");
    ClientConfig::cfg.portTcpDown = pt.get<int>("network.port_tcp_down");
    ClientConfig::cfg.portUdp = pt.get<int>("network.port_udp");

    // Cores
    if (const auto core = pt.get_optional<int>("cpu.core_system")) {
      ClientConfig::cfg.coreSystem = *core;
    }
    if (const auto cores = pt.get_optional<String>("cpu.cores_network")) {
      ClientConfig::cfg.coresNetwork = parseCores(*cores);
    }
    if (const auto cores = pt.get_optional<String>("cpu.cores_app")) {
      ClientConfig::cfg.coresApp = parseCores(*cores);
    }

    // Rates
    ClientConfig::cfg.tradeRate = Microseconds(pt.get<int>("rates.trade_rate"));
    ClientConfig::cfg.monitorRate = Seconds(pt.get<int>("rates.monitor_rate"));

    // Credentials
    ClientConfig::cfg.name = pt.get<String>("credentials.name");
    ClientConfig::cfg.password = pt.get<String>("credentials.password");

    // kafka
    ClientConfig::cfg.kafkaBroker = pt.get<String>("kafka.kafka_broker");
    ClientConfig::cfg.kafkaConsumerGroup = pt.get<String>("kafka.kafka_consumer_group");
    ClientConfig::cfg.kafkaPollRate = Milliseconds(pt.get<int>("kafka.kafka_poll_rate"));

    // Logging
    ClientConfig::cfg.logLevel = utils::fromString<LogLevel>(pt.get<String>("log.level"));
    ClientConfig::cfg.logOutput = pt.get<String>("log.output");

    verifyConfig(ClientConfig::cfg);
  }

  static void verifyConfig(CRef<ClientConfig> cfg) {
    if (cfg.url.empty() || cfg.portTcpUp == 0 || cfg.portTcpDown == 0 || cfg.portUdp == 0) {
      throw std::runtime_error("Invalid network configuration");
    }
    if (utils::hasIntersection(cfg.coresApp, cfg.coresNetwork)) {
      throw std::runtime_error("Invalid cores configuration");
    }
    if (cfg.coreSystem.has_value() && std::find(cfg.coresApp.begin(), cfg.coresApp.end(),
                                                cfg.coreSystem.value()) != cfg.coresApp.end()) {
      throw std::runtime_error("Invalid cores configuration");
    }
    if (cfg.coreSystem.has_value() && std::find(cfg.coresNetwork.begin(), cfg.coresNetwork.end(),
                                                cfg.coreSystem.value()) != cfg.coresNetwork.end()) {
      throw std::runtime_error("Invalid cores configuration");
    }
    if (cfg.tradeRate.count() == 0 || cfg.monitorRate.count() == 0) {
      throw std::runtime_error("Invalid rates configuration");
    }
    if (cfg.name.empty() || cfg.password.empty()) {
      throw std::runtime_error("Invalid credentials");
    }
    if (cfg.logOutput.empty()) {
      throw std::runtime_error("Invalid log file");
    }
    if (cfg.kafkaBroker.empty() || cfg.kafkaConsumerGroup.empty()) {
      throw std::runtime_error("Invalid kafka configuration");
    }
  }

  static ByteBuffer parseCores(CRef<String> input) {
    ByteBuffer result;
    std::stringstream ss(input);
    String token;

    while (std::getline(ss, token, ',')) {
      result.push_back(static_cast<uint8_t>(std::stoi(token)));
    }
    return result;
  }
};

} // namespace hft::client

#endif // HFT_CLIENT_CONFIGREADER_HPP