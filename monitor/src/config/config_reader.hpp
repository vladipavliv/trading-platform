/**
 * @author Vladimir Pavliv
 * @date 2025-03-30
 */

#ifndef HFT_MONITOR_CONFIGREADER_HPP
#define HFT_MONITOR_CONFIGREADER_HPP

#include <sstream>
#include <vector>

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "config.hpp"
#include "network_types.hpp"
#include "types.hpp"
#include "utils/template_utils.hpp"

namespace hft::monitor {

struct ConfigReader {
  static void readConfig(CRef<String> fileName) {
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(fileName, pt);
    // kafka
    Config::cfg.kafkaBroker = pt.get<String>("kafka.kafka_broker");
    Config::cfg.kafkaConsumerGroup = pt.get<String>("kafka.kafka_consumer_group");
    Config::cfg.kafkaPollRate = Milliseconds(pt.get<int>("kafka.kafka_poll_rate"));

    // Logging
    Config::cfg.logLevel = utils::fromString<LogLevel>(pt.get<std::string>("log.level"));
    Config::cfg.logOutput = pt.get<String>("log.output");

    verifyConfig(Config::cfg);
  }

  static void verifyConfig(CRef<Config> cfg) {
    if (cfg.kafkaBroker.empty() || cfg.kafkaConsumerGroup.empty()) {
      throw std::runtime_error("Invalid kafka configuration");
    }
  }
};

} // namespace hft::monitor

#endif // HFT_MONITOR_CONFIGREADER_HPP