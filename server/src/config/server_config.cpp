/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#include "server_config.hpp"
#include "logging.hpp"
#include "ptr_types.hpp"
#include "utils/parse_utils.hpp"
#include "utils/string_utils.hpp"
#include "utils/time_utils.hpp"

namespace hft::server {

void ServerConfig::load(CRef<String> fileName) {
  Config::load(fileName);

  // Network
  url = Config::get<String>("network.url");
  portTcpUp = Config::get<size_t>("network.port_tcp_up");
  portTcpDown = Config::get<size_t>("network.port_tcp_down");
  portUdp = Config::get<size_t>("network.port_udp");

  // Cores
  if (const auto core = Config::get_optional<uint16_t>("cpu.core_system")) {
    coreSystem = *core;
    if (*core == 0) {
      throw std::runtime_error("Invalid cores configuration");
    }
  }
  if (const auto core = Config::get_optional<uint16_t>("cpu.core_network")) {
    coreNetwork = *core;
    if (coreSystem.has_value() && coreSystem == *core) {
      throw std::runtime_error("Invalid cores configuration");
    }
    if (*core == 0) {
      throw std::runtime_error("Invalid cores configuration");
    }
  }
  if (const auto core = Config::get_optional<uint16_t>("cpu.core_gateway")) {
    coreGateway = *core;
    if (coreSystem.has_value() && coreSystem == *core) {
      throw std::runtime_error("Invalid cores configuration");
    }
    if (coreNetwork.has_value() && coreNetwork == *core) {
      throw std::runtime_error("Invalid cores configuration");
    }
    if (*core == 0) {
      throw std::runtime_error("Invalid cores configuration");
    }
  }
  if (const auto cores = Config::get_optional<String>("cpu.cores_app")) {
    coresApp = utils::split<CoreId>(*cores);
    if (coreSystem.has_value() &&
        std::find(coresApp.begin(), coresApp.end(), *coreSystem) != coresApp.end()) {
      throw std::runtime_error("Invalid cores configuration");
    }
    if (coreNetwork.has_value() &&
        std::find(coresApp.begin(), coresApp.end(), *coreNetwork) != coresApp.end()) {
      throw std::runtime_error("Invalid cores configuration");
    }
  }

  nsPerCycle = utils::getNsPerCycle();

  // Rates
  priceFeedRate = Config::get<uint32_t>("rates.price_feed_rate_us");
  monitorRate = Config::get<uint32_t>("rates.monitor_rate_ms");
  telemetryRate = Config::get<uint32_t>("rates.telemetry_ms");

  // Logging
  logOutput = Config::get<String>("log.output");
}

void ServerConfig::log() {
  LOG_INFO_SYSTEM("Url:{} TcpUp:{} TcpDown:{} Udp:{}", url, portTcpUp, portTcpDown, portUdp);
  LOG_INFO_SYSTEM("SystemCore:{} NetworkCore:{} GatewayCore:{} AppCores:{} PriceFeedRate:{}µs",
                  coreSystem.value_or(0), coreNetwork.value_or(0), coreGateway.value_or(0),
                  toString(coresApp), priceFeedRate);
  LOG_INFO_SYSTEM("LogOutput: {}", logOutput);
}

} // namespace hft::server
