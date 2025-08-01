/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_SERVER_CONFIG_HPP
#define HFT_SERVER_CONFIG_HPP

#include "boost_types.hpp"
#include "config/config.hpp"
#include "logging.hpp"
#include "server_config.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft::server {

struct ServerConfig {
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
  Microseconds priceFeedRate;
  Seconds monitorRate;
  size_t orderBookLimit;

  // Logging
  LogLevel logLevel;
  String logOutput;

  static ServerConfig cfg;

  static void load(CRef<String> fileName) {
    Config::load(fileName);

    // Network
    ServerConfig::cfg.url = Config::get<String>("network.url");
    ServerConfig::cfg.portTcpUp = Config::get<size_t>("network.port_tcp_up");
    ServerConfig::cfg.portTcpDown = Config::get<size_t>("network.port_tcp_down");
    ServerConfig::cfg.portUdp = Config::get<size_t>("network.port_udp");

    // Cores
    if (const auto core = Config::get_optional<size_t>("cpu.core_system")) {
      ServerConfig::cfg.coreSystem = *core;
    }
    if (const auto cores = Config::get_optional<String>("cpu.cores_network")) {
      ServerConfig::cfg.coresNetwork = utils::split(*cores);
    }
    if (const auto cores = Config::get_optional<String>("cpu.cores_app")) {
      ServerConfig::cfg.coresApp = utils::split(*cores);
    }

    // Rates
    ServerConfig::cfg.priceFeedRate = Microseconds(Config::get<size_t>("rates.price_feed_rate_us"));
    ServerConfig::cfg.monitorRate = Seconds(Config::get<size_t>("rates.monitor_rate_s"));
    ServerConfig::cfg.orderBookLimit = Config::get<size_t>("rates.order_book_limit");

    // Logging
    ServerConfig::cfg.logLevel = utils::fromString<LogLevel>(Config::get<String>("log.level"));
    ServerConfig::cfg.logOutput = Config::get<String>("log.output");

    verify();
  }

  static void verify() {
    if (cfg.url.empty() || cfg.portTcpUp == 0 || cfg.portTcpDown == 0 || cfg.portUdp == 0) {
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
    if (cfg.monitorRate.count() == 0) {
      throw std::runtime_error("Invalid rates configuration");
    }
    if (cfg.logOutput.empty()) {
      throw std::runtime_error("Invalid log file");
    }
  }

  static void log() {
    LOG_INFO_SYSTEM("Url:{} TcpUp:{} TcpDown:{} Udp:{}", cfg.url, cfg.portTcpUp, cfg.portTcpDown,
                    cfg.portUdp);
    LOG_INFO_SYSTEM("SystemCore:{} NetworkCores:{} AppCores:{} PriceFeedRate:{}us",
                    cfg.coreSystem.value_or(0), utils::toString(cfg.coresNetwork),
                    utils::toString(cfg.coresApp), cfg.priceFeedRate.count());
    LOG_INFO_SYSTEM("LogLevel: {} LogOutput: {}", utils::toString(cfg.logLevel), cfg.logOutput);
  }
};

ServerConfig ServerConfig::cfg;

} // namespace hft::server

#endif // HFT_SERVER_CONFIG_HPP