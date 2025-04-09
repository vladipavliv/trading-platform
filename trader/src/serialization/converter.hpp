/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_TRADER_SERIALIZATION_CONVERTER_HPP
#define HFT_TRADER_SERIALIZATION_CONVERTER_HPP

#include "gen/tradercmd_generated.h"
#include "logging.hpp"
#include "market_types.hpp"
#include "trader_command.hpp"
#include "types.hpp"

namespace hft::trader::serialization::fbs {

static inline Ticker fbStringToTicker(const flatbuffers::String *str) {
  Ticker ticker{};
  const auto size = std::min((size_t)str->size(), TICKER_SIZE);
  std::memcpy(ticker.data(), str->data(), size);
  return ticker;
}

static inline String fbStringToString(const flatbuffers::String *str) {
  return String(str->c_str(), str->size());
}

using TraderCommandType = hft::serialization::gen::fbs::TraderCommandType;

TraderCommand convert(TraderCommandType cmd) {
  switch (cmd) {
  case TraderCommandType::TraderCommandType_TradeStart:
    return TraderCommand::TradeStart;
  case TraderCommandType::TraderCommandType_TradeStop:
    return TraderCommand::TradeStop;
  case TraderCommandType::TraderCommandType_TradeSpeedUp:
    return TraderCommand::TradeSpeedUp;
  case TraderCommandType::TraderCommandType_TradeSpeedDown:
    return TraderCommand::TradeSpeedDown;
  case TraderCommandType::TraderCommandType_Shutdown:
    return TraderCommand::Shutdown;
  default:
    LOG_ERROR("Unknown TraderCommand {}", (uint8_t)cmd);
    return static_cast<TraderCommand>(cmd);
  }
}

TraderCommandType convert(TraderCommand cmd) {
  switch (cmd) {
  case TraderCommand::TradeStart:
    return TraderCommandType::TraderCommandType_TradeStart;
  case TraderCommand::TradeStop:
    return TraderCommandType::TraderCommandType_TradeStop;
  case TraderCommand::TradeSpeedUp:
    return TraderCommandType::TraderCommandType_TradeSpeedUp;
  case TraderCommand::TradeSpeedDown:
    return TraderCommandType::TraderCommandType_TradeSpeedDown;
  case TraderCommand::Shutdown:
    return TraderCommandType::TraderCommandType_Shutdown;
  default:
    LOG_ERROR("Unknown TraderCommandType {}", (uint8_t)cmd);
    return static_cast<TraderCommandType>(cmd);
  }
}

} // namespace hft::trader::serialization::fbs

#endif // HFT_TRADER_SERIALIZATION_CONVERTER_HPP