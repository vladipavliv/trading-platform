/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_SERVERCOMMAND_HPP
#define HFT_SERVER_SERVERCOMMAND_HPP

#include <stdint.h>

#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft::server {
/**
 * @brief Commands for console management
 * @todo Make commands to add/remove workers, orders rerouting
 */
enum class ServerCommand : uint8_t {
  PriceFeedStart,
  PriceFeedStop,
  KafkaFeedStart,
  KafkaFeedStop,
  Shutdown
};
} // namespace hft::server

namespace hft::utils {
inline String toString(const server::ServerCommand &command) {
  using namespace server;
  switch (command) {
  case ServerCommand::PriceFeedStart:
    return "start price feed";
  case ServerCommand::PriceFeedStop:
    return "stop price feed";
  case ServerCommand::KafkaFeedStart:
    return "start kafka feed";
  case ServerCommand::KafkaFeedStop:
    return "stop kafka feed";
  case ServerCommand::Shutdown:
    return "shutdown";
  default:
    return std::format("unknown command {}", static_cast<uint8_t>(command));
  }
}
} // namespace hft::utils

#endif // HFT_SERVER_SERVERCOMMAND_HPP
