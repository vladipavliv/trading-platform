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
    Config::cfg.coreIds = parseCores(pt.get<std::string>("cpu.core_ids"));

    // Rates
    Config::cfg.tradeRateUs = pt.get<int>("rates.trade_rate");
    Config::cfg.priceFeedRateUs = pt.get<int>("rates.price_feed_rate");
    Config::cfg.monitorRateS = pt.get<int>("rates.monitor_rate");

    // Logging
    Config::cfg.logLevel = utils::fromString<LogLevel>(pt.get<std::string>("log.level"));
    Config::cfg.logOutput = pt.get<std::string>("log.output");
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