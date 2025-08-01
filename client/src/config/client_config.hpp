/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_CLIENT_CONFIG_HPP
#define HFT_CLIENT_CONFIG_HPP

#include "boost_types.hpp"
#include "config/config.hpp"
#include "logging.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::client {

struct ClientConfig {
  // Network
  String url;
  Port portTcpUp;
  Port portTcpDown;
  Port portUdp;

  // Cores
  Optional<CoreId> coreSystem;
  std::vector<CoreId> coresNetwork;
  std::vector<CoreId> coresApp;

  // Rates
  Microseconds tradeRate;
  Seconds monitorRate;

  // Credentials
  String name;
  String password;

  // Logging
  LogLevel logLevel;
  String logOutput;

  static ClientConfig cfg;

  static void load(CRef<String> fileName) {
    Config::load(fileName);

    // Network config
    ClientConfig::cfg.url = Config::get<String>("network.url");
    ClientConfig::cfg.portTcpUp = Config::get<size_t>("network.port_tcp_up");
    ClientConfig::cfg.portTcpDown = Config::get<size_t>("network.port_tcp_down");
    ClientConfig::cfg.portUdp = Config::get<size_t>("network.port_udp");

    // Cores
    if (const auto core = Config::get_optional<size_t>("cpu.core_system")) {
      ClientConfig::cfg.coreSystem = *core;
    }
    if (const auto cores = Config::get_optional<String>("cpu.cores_network")) {
      ClientConfig::cfg.coresNetwork = utils::split(*cores);
    }
    if (const auto cores = Config::get_optional<String>("cpu.cores_app")) {
      ClientConfig::cfg.coresApp = utils::split(*cores);
    }

    // Rates
    ClientConfig::cfg.tradeRate = Microseconds(Config::get<size_t>("rates.trade_rate_us"));
    ClientConfig::cfg.monitorRate = Seconds(Config::get<size_t>("rates.monitor_rate_s"));

    // Credentials
    ClientConfig::cfg.name = Config::get<String>("credentials.name");
    ClientConfig::cfg.password = Config::get<String>("credentials.password");

    // Logging
    ClientConfig::cfg.logLevel = utils::fromString<LogLevel>(Config::get<String>("log.level"));
    ClientConfig::cfg.logOutput = Config::get<String>("log.output");

    verify();
  }

  static void verify() {
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
    if (cfg.monitorRate.count() == 0) {
      throw std::runtime_error("Invalid rates configuration");
    }
    if (cfg.name.empty() || cfg.password.empty()) {
      throw std::runtime_error("Invalid credentials");
    }
    if (cfg.logOutput.empty()) {
      throw std::runtime_error("Invalid log file");
    }
  }

  static void log() {
    LOG_INFO_SYSTEM("Url:{} TcpUp:{} TcpDown:{} Udp:{}", cfg.url, cfg.portTcpUp, cfg.portTcpDown,
                    cfg.portUdp);
    LOG_INFO_SYSTEM("SystemCore:{} NetworkCores:{} AppCores:{} TradeRate:{}us",
                    cfg.coreSystem.value_or(0), utils::toString(cfg.coresNetwork),
                    utils::toString(cfg.coresApp), cfg.tradeRate.count());
    LOG_INFO_SYSTEM("LogLevel: {} LogOutput: {}", utils::toString(cfg.logLevel), cfg.logOutput);
    LOG_INFO_SYSTEM("Name: {} Password: {}", cfg.name, cfg.password);
  }
};

ClientConfig ClientConfig::cfg;

} // namespace hft::client

#endif // HFT_CLIENT_CONFIG_HPP