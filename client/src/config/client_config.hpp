/**
 * @author Vladimir Pavliv
 * @date 2025-02-10
 */

#ifndef HFT_CLIENT_CONFIG_HPP
#define HFT_CLIENT_CONFIG_HPP

#include "config/config.hpp"
#include "functional_types.hpp"
#include "primitive_types.hpp"

namespace hft::client {

struct ClientConfig {
  static ClientConfig cfg;

  // Network
  String url;
  Port portTcpUp;
  Port portTcpDown;
  Port portUdp;

  // Cores
  Optional<CoreId> coreSystem;
  Optional<CoreId> coreNetwork;
  std::vector<CoreId> coresApp;
  double nsPerCycle;

  // Rates
  uint32_t tradeRate;
  uint32_t monitorRate;
  uint32_t telemetryTate;

  // Credentials
  String name;
  String password;

  // Logging
  String logOutput;

  static void load(const String &fileName);
  static void log();
};

} // namespace hft::client

#endif // HFT_CLIENT_CONFIG_HPP