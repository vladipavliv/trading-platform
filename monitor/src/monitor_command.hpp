/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_MONITOR_SERVERCOMMAND_HPP
#define HFT_MONITOR_SERVERCOMMAND_HPP

#include <stdint.h>

#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft::monitor {
/**
 * @brief
 */
enum class MonitorCommand : uint8_t {
  ServerPriceFeedStart,
  ServerPriceFeedStop,
  ServerKafkaFeedStart,
  ServerKafkaFeedStop,
  ServerShutdown,
  TraderTradeStart,
  TraderTradeStop,
  TraderTradeSpeedUp,
  TraderTradeSpeedDown,
  TraderShutdown
};
} // namespace hft::monitor

namespace hft::utils {
template <>
std::string toString<monitor::MonitorCommand>(const monitor::MonitorCommand &command) {
  using namespace monitor;
  switch (command) {
  case MonitorCommand::ServerPriceFeedStart:
    return "server start price feed";
  case MonitorCommand::ServerPriceFeedStop:
    return "server stop price feed";
  case MonitorCommand::ServerKafkaFeedStart:
    return "server start kafka feed";
  case MonitorCommand::ServerKafkaFeedStop:
    return "server stop kafka feed";
  case MonitorCommand::ServerShutdown:
    return "server shutdown";
  case MonitorCommand::TraderTradeStart:
    return "trader start price feed";
  case MonitorCommand::TraderTradeStop:
    return "trader stop price feed";
  case MonitorCommand::TraderTradeSpeedUp:
    return "trader start kafka feed";
  case MonitorCommand::TraderTradeSpeedDown:
    return "trader stop kafka feed";
  case MonitorCommand::TraderShutdown:
    return "trader shutdown";
  default:
    return std::format("unknown command {}", static_cast<uint8_t>(command));
  }
}
} // namespace hft::utils

#endif // HFT_SERVER_SERVERCOMMAND_HPP
