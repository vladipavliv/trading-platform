/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_SERIALIZATION_CONVERTER_HPP
#define HFT_COMMON_SERIALIZATION_CONVERTER_HPP

#include "gen/servercmd_generated.h"
#include "logging.hpp"
#include "market_types.hpp"
#include "server_command.hpp"
#include "types.hpp"

namespace hft::server::serialization::fbs {

static inline Ticker fbStringToTicker(const flatbuffers::String *str) {
  Ticker ticker{};
  const auto size = std::min((size_t)str->size(), TICKER_SIZE);
  std::memcpy(ticker.data(), str->data(), size);
  return ticker;
}

static inline String fbStringToString(const flatbuffers::String *str) {
  return String(str->c_str(), str->size());
}

using ServerCommandType = hft::serialization::gen::fbs::ServerCommandType;

ServerCommand convert(ServerCommandType cmd) {
  switch (cmd) {
  case ServerCommandType::ServerCommandType_PriceFeedStart:
    return ServerCommand::PriceFeedStart;
  case ServerCommandType::ServerCommandType_PriceFeedStop:
    return ServerCommand::PriceFeedStop;
  case ServerCommandType::ServerCommandType_KafkaFeedStart:
    return ServerCommand::KafkaFeedStart;
  case ServerCommandType::ServerCommandType_KafkaFeedStop:
    return ServerCommand::KafkaFeedStop;
  case ServerCommandType::ServerCommandType_Shutdown:
    return ServerCommand::Shutdown;
  default:
    LOG_ERROR("Unknown ServerCommand {}", (uint8_t)cmd);
    return static_cast<ServerCommand>(cmd);
  }
}

ServerCommandType convert(ServerCommand cmd) {
  switch (cmd) {
  case ServerCommand::PriceFeedStart:
    return ServerCommandType::ServerCommandType_PriceFeedStart;
  case ServerCommand::PriceFeedStop:
    return ServerCommandType::ServerCommandType_PriceFeedStop;
  case ServerCommand::KafkaFeedStart:
    return ServerCommandType::ServerCommandType_KafkaFeedStart;
  case ServerCommand::KafkaFeedStop:
    return ServerCommandType::ServerCommandType_KafkaFeedStop;
  case ServerCommand::Shutdown:
    return ServerCommandType::ServerCommandType_Shutdown;
  default:
    LOG_ERROR("Unknown ServerCommandType {}", (uint8_t)cmd);
    return static_cast<ServerCommandType>(cmd);
  }
}

} // namespace hft::server::serialization::fbs

#endif // HFT_COMMON_SERIALIZATION_CONVERTER_HPP