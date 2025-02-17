/**
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

    // Network config
    Config::cfg.url = pt.get<std::string>("network.url");
    Config::cfg.portTcpIn = pt.get<int>("network.port_tcp_in");
    Config::cfg.portTcpOut = pt.get<int>("network.port_tcp_out");
    Config::cfg.portUdp = pt.get<int>("network.port_udp");

    // Cpu
    Config::cfg.threadsIo = pt.get<int>("cpu.threads_io");
    Config::cfg.threadsApp = pt.get<int>("cpu.threads_event");
    Config::cfg.threadsPin = pt.get<bool>("cpu.thread_pin");
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

  static ByteBuffer parseCores(StringRef input) {
    ByteBuffer result;
    std::stringstream ss(input);
    std::string token;

    while (std::getline(ss, token, ',')) {
      result.push_back(static_cast<uint8_t>(std::stoi(token)));
    }
    return result;
  }
#endif
};

} // namespace hft

#endif // HFT_COMMON_CONFIGREADER_HPP