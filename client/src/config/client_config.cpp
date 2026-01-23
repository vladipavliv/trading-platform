/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#include "client_config.hpp"
#include "logging.hpp"
#include "ptr_types.hpp"
#include "utils/parse_utils.hpp"
#include "utils/string_utils.hpp"
#include "utils/time_utils.hpp"

namespace hft::client {

ClientConfig::ClientConfig(CRef<String> fileName) : data{fileName} {
  // Network config
  url = data.get<String>("network.url");
  portTcpUp = data.get<size_t>("network.port_tcp_up");
  portTcpDown = data.get<size_t>("network.port_tcp_down");
  portUdp = data.get<size_t>("network.port_udp");

  // Cores
  if (const auto core = data.get_optional<size_t>("cpu.core_system")) {
    coreSystem = *core;
    if (*core == 0) {
      throw std::runtime_error("Invalid cores configuration");
    }
  }
  if (const auto core = data.get_optional<size_t>("cpu.core_network")) {
    coreNetwork = *core;
    if (coreSystem.has_value() && coreSystem == *core) {
      throw std::runtime_error("Invalid cores configuration");
    }
    if (*core == 0) {
      throw std::runtime_error("Invalid cores configuration");
    }
  }
  if (const auto cores = data.get_optional<String>("cpu.cores_app")) {
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

  // Rates
  tradeRate = data.get<size_t>("rates.trade_rate_us");
  monitorRate = data.get<size_t>("rates.monitor_rate_ms");
  telemetryTate = data.get<size_t>("rates.telemetry_ms");

  // Credentials
  name = data.get<String>("credentials.name");
  password = data.get<String>("credentials.password");

  // Logging
  logOutput = data.get<String>("log.output");
}

void ClientConfig::print() const {
  LOG_INFO_SYSTEM("Url:{} TcpUp:{} TcpDown:{} Udp:{}", url, portTcpUp, portTcpDown, portUdp);
  LOG_INFO_SYSTEM("SystemCore:{} NetworkCore:{} AppCores:{} TradeRate:{}us", coreSystem.value_or(0),
                  coreNetwork.value_or(0), toString(coresApp), tradeRate);
  LOG_INFO_SYSTEM("LogOutput: {}", logOutput);
  LOG_INFO_SYSTEM("Name: {} Password: {}", name, password);
}

} // namespace hft::client
