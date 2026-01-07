/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_SERVER_CONFIG_HPP
#define HFT_SERVER_CONFIG_HPP

#include "config/config.hpp"
#include "constants.hpp"
#include "functional_types.hpp"
#include "primitive_types.hpp"

namespace hft::server {

struct ServerConfig {
  static ServerConfig cfg;

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
  uint32_t priceFeedRate;
  uint32_t monitorRate;
  uint32_t telemetryRate;

  // Data
  size_t orderBookLimit;
  bool orderBookPersist;

  // Logging
  String logOutput;

  static void load(CRef<String> fileName);
  static void log();
};

} // namespace hft::server

#endif // HFT_SERVER_CONFIG_HPP