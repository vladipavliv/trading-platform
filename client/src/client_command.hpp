/**
 * @author Vladimir Pavliv
 * @date 2025-03-06
 */

#ifndef HFT_SERVER_ClientCommand_HPP
#define HFT_SERVER_ClientCommand_HPP

#include <stdint.h>

#include "utils/string_utils.hpp"

namespace hft::client {
/**
 * @brief Command for console management
 */
enum class ClientCommand : uint8_t {
  TradeStart,
  TradeStop,
  TradeSpeedUp,
  TradeSpeedDown,
  KafkaFeedStart,
  KafkaFeedStop,
  Shutdown
};
} // namespace hft::client

namespace hft::utils {

template <>
std::string toString<client::ClientCommand>(const client::ClientCommand &command) {
  using namespace client;
  switch (command) {
  case ClientCommand::TradeStart:
    return "trade start";
  case ClientCommand::TradeStop:
    return "trade stop";
  case ClientCommand::TradeSpeedUp:
    return "+ trade speed";
  case ClientCommand::TradeSpeedDown:
    return "- trade speed";
  case ClientCommand::KafkaFeedStart:
    return "start kafka feed";
  case ClientCommand::KafkaFeedStop:
    return "stop kafka feed";
  case ClientCommand::Shutdown:
    return "shutdown";
  default:
    return std::format("unknown command {}", static_cast<uint8_t>(command));
  }
}

} // namespace hft::utils

#endif // HFT_SERVER_ClientCommand_HPP
