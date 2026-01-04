/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_SERVER_CONFIG_HPP
#define HFT_SERVER_CONFIG_HPP

#include "boost_types.hpp"
#include "config/config.hpp"
#include "constants.hpp"
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
  Optional<CoreId> coreNetwork;
  std::vector<CoreId> coresApp;

  // Rates
  Microseconds priceFeedRate;
  Milliseconds monitorRate;
  Milliseconds telemetryRate;

  // Data
  size_t orderBookLimit;
  bool orderBookPersist;

  // Logging
  String logOutput;

  static ServerConfig cfg;

  static void load(CRef<String> fileName) {
    Config::load(fileName);

    // Network
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
      cfg.coresApp = utils::split(*cores);
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
    cfg.priceFeedRate = Microseconds(Config::get<size_t>("rates.price_feed_rate_us"));
    cfg.monitorRate = Milliseconds(Config::get<size_t>("rates.monitor_rate_ms"));
    cfg.telemetryRate = Milliseconds(Config::get<size_t>("rates.telemetry_ms"));

    // Data
    cfg.orderBookLimit =
        Config::get_optional<size_t>("data.order_book_limit").value_or(ORDER_BOOK_LIMIT);
    cfg.orderBookPersist = Config::get_optional<bool>("data.order_book_persist").value_or(false);

    // Logging
    cfg.logOutput = Config::get<String>("log.output");
  }

  static void log() {
    LOG_INFO_SYSTEM("Url:{} TcpUp:{} TcpDown:{} Udp:{}", cfg.url, cfg.portTcpUp, cfg.portTcpDown,
                    cfg.portUdp);
    LOG_INFO_SYSTEM("SystemCore:{} NetworkCore:{} AppCores:{} PriceFeedRate:{}us",
                    cfg.coreSystem.value_or(0), cfg.coreNetwork.value_or(0),
                    utils::toString(cfg.coresApp), cfg.priceFeedRate.count());
    LOG_INFO_SYSTEM("OrderBookLimit: {} OrderBookPersist: {}", cfg.orderBookLimit,
                    cfg.orderBookPersist);
    LOG_INFO_SYSTEM("LogOutput: {}", cfg.logOutput);
  }
};

inline ServerConfig ServerConfig::cfg;

} // namespace hft::server

#endif // HFT_SERVER_CONFIG_HPP