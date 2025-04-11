/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_TRADER_CONFIGREADER_HPP
#define HFT_TRADER_CONFIGREADER_HPP

#include <sstream>
#include <vector>

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "network_types.hpp"
#include "trader_config.hpp"
#include "types.hpp"
#include "utils/template_utils.hpp"

namespace hft::trader {

struct TraderConfigReader {
  static void readConfig(CRef<String> fileName) {
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(fileName, pt);

    // Network config
    TraderConfig::cfg.url = pt.get<String>("network.url");
    TraderConfig::cfg.portTcpUp = pt.get<int>("network.port_tcp_up");
    TraderConfig::cfg.portTcpDown = pt.get<int>("network.port_tcp_down");
    TraderConfig::cfg.portUdp = pt.get<int>("network.port_udp");

    // Cores
    if (const auto core = pt.get_optional<int>("cpu.core_system")) {
      TraderConfig::cfg.coreSystem = *core;
    }
    if (const auto cores = pt.get_optional<String>("cpu.cores_network")) {
      TraderConfig::cfg.coresNetwork = parseCores(*cores);
    }
    if (const auto cores = pt.get_optional<String>("cpu.cores_app")) {
      TraderConfig::cfg.coresApp = parseCores(*cores);
    }

    // Rates
    TraderConfig::cfg.tradeRate = Microseconds(pt.get<int>("rates.trade_rate"));
    TraderConfig::cfg.monitorRate = Seconds(pt.get<int>("rates.monitor_rate"));

    // Credentials
    TraderConfig::cfg.name = pt.get<String>("credentials.name");
    TraderConfig::cfg.password = pt.get<String>("credentials.password");

    // kafka
    TraderConfig::cfg.kafkaBroker = pt.get<String>("kafka.kafka_broker");
    TraderConfig::cfg.kafkaConsumerGroup = pt.get<String>("kafka.kafka_consumer_group");
    TraderConfig::cfg.kafkaPollRate = Milliseconds(pt.get<int>("kafka.kafka_poll_rate"));

    // Logging
    TraderConfig::cfg.logLevel = utils::fromString<LogLevel>(pt.get<String>("log.level"));
    TraderConfig::cfg.logOutput = pt.get<String>("log.output");

    verifyConfig(TraderConfig::cfg);
  }

  static void verifyConfig(CRef<TraderConfig> cfg) {
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

} // namespace hft::trader

#endif // HFT_TRADER_CONFIGREADER_HPP