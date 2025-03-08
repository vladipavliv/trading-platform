/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_COMMON_CONFIGREADER_HPP
#define HFT_COMMON_CONFIGREADER_HPP

#include <spdlog/spdlog.h>
#include <sstream>
#include <vector>

#ifdef __cpp_rtti
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#endif

#include "config.hpp"
#include "network_types.hpp"
#include "types.hpp"
#include "utils/template_utils.hpp"

namespace hft {

struct ConfigReader {

#ifdef __cpp_rtti
  static void readConfig(const std::string &fileName) {
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(fileName, pt);

    // Network config
    Config::cfg.url = pt.get<std::string>("network.url");
    Config::cfg.portTcpIn = pt.get<int>("network.port_tcp_in");
    Config::cfg.portTcpOut = pt.get<int>("network.port_tcp_out");
    Config::cfg.portUdp = pt.get<int>("network.port_udp");

    // Cpu
    Config::cfg.coreSystem = pt.get<int>("cpu.core_system");
    Config::cfg.coresNetwork = parseCores(pt.get<std::string>("cpu.cores_network"));
    Config::cfg.coresApp = parseCores(pt.get<std::string>("cpu.cores_app"));

    // Rates
    Config::cfg.tradeRate = Microseconds(pt.get<int>("rates.trade_rate"));
    Config::cfg.priceFeedRate = Microseconds(pt.get<int>("rates.price_feed_rate"));
    Config::cfg.monitorRate = Seconds(pt.get<int>("rates.monitor_rate"));

    // Logging
    Config::cfg.logLevel = utils::fromString<LogLevel>(pt.get<std::string>("log.level"));
    Config::cfg.logOutput = pt.get<std::string>("log.output");

    verifyConfig(Config::cfg);
  }

  static void verifyConfig(CRef<Config> cfg) {
    if (cfg.url.empty() || cfg.portTcpIn == 0 || cfg.portTcpOut == 0 || cfg.portUdp == 0) {
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
    if (cfg.tradeRate.count() == 0 || cfg.priceFeedRate.count() == 0 ||
        cfg.monitorRate.count() == 0) {
      throw std::runtime_error("Invalid rates configuration");
    }
    if (cfg.logOutput.empty()) {
      throw std::runtime_error("Invalid log file");
    }
  }
#else
  static void readConfig() {
    Config::cfg.url = SERVER_URL;
    Config::cfg.portTcpIn = PORT_TCP_IN;
    Config::cfg.portTcpOut = PORT_TCP_OUT;
    Config::cfg.portUdp = PORT_UDP;
    Config::cfg.coresIo = parseCores(CORES_IO);
    Config::cfg.coresApp = parseCores(CORES_APP);
  }
#endif
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

} // namespace hft

#endif // HFT_COMMON_CONFIGREADER_HPP