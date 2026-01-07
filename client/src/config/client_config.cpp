/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#include "client_config.hpp"
#include "logging.hpp"
#include "utils/parse_utils.hpp"
#include "utils/string_utils.hpp"

namespace hft::client {

ClientConfig ClientConfig::cfg;

void ClientConfig::load(CRef<String> fileName) {
  Config::load(fileName);

  // Network config
  cfg.url = Config::get<String>("network.url");
  cfg.portTcpUp = Config::get<size_t>("network.port_tcp_up");
  cfg.portTcpDown = Config::get<size_t>("network.port_tcp_down");
  cfg.portUdp = Config::get<size_t>("network.port_udp");

  // Cores
  if (const auto core = Config::get_optional<size_t>("cpu.core_system")) {
    cfg.coreSystem = *core;
    if (*core == 0) {
      throw std::runtime_error("Invalid cores configuration");
    }
  }
  if (const auto core = Config::get_optional<size_t>("cpu.core_network")) {
    cfg.coreNetwork = *core;
    if (cfg.coreSystem.has_value() && cfg.coreSystem == *core) {
      throw std::runtime_error("Invalid cores configuration");
    }
    if (*core == 0) {
      throw std::runtime_error("Invalid cores configuration");
    }
  }
  if (const auto cores = Config::get_optional<String>("cpu.cores_app")) {
    cfg.coresApp = utils::split<CoreId>(*cores);
    if (cfg.coreSystem.has_value() && std::find(cfg.coresApp.begin(), cfg.coresApp.end(),
                                                *cfg.coreSystem) != cfg.coresApp.end()) {
      throw std::runtime_error("Invalid cores configuration");
    }
    if (cfg.coreNetwork.has_value() && std::find(cfg.coresApp.begin(), cfg.coresApp.end(),
                                                 *cfg.coreNetwork) != cfg.coresApp.end()) {
      throw std::runtime_error("Invalid cores configuration");
    }
  }

  // Rates
  cfg.tradeRate = Config::get<size_t>("rates.trade_rate_us");
  cfg.monitorRate = Config::get<size_t>("rates.monitor_rate_ms");
  cfg.telemetryTate = Config::get<size_t>("rates.telemetry_ms");

  // Credentials
  cfg.name = Config::get<String>("credentials.name");
  cfg.password = Config::get<String>("credentials.password");

  // Logging
  cfg.logOutput = Config::get<String>("log.output");
}

void ClientConfig::log() {
  LOG_INFO_SYSTEM("Url:{} TcpUp:{} TcpDown:{} Udp:{}", cfg.url, cfg.portTcpUp, cfg.portTcpDown,
                  cfg.portUdp);
  LOG_INFO_SYSTEM("SystemCore:{} NetworkCore:{} AppCores:{} TradeRate:{}us",
                  cfg.coreSystem.value_or(0), cfg.coreNetwork.value_or(0), toString(cfg.coresApp),
                  cfg.tradeRate);
  LOG_INFO_SYSTEM("LogOutput: {}", cfg.logOutput);
  LOG_INFO_SYSTEM("Name: {} Password: {}", cfg.name, cfg.password);
}

} // namespace hft::client
