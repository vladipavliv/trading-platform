/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_CONVERTER_HPP
#define HFT_COMMON_CONVERTER_HPP

#include "gen/marketdata_generated.h"
#include "gen/metadata_generated.h"
#include "logging.hpp"
#include "market_types.hpp"
#include "types.hpp"

namespace hft::serialization {

static inline Ticker fbStringToTicker(const flatbuffers::String *str) {
  Ticker ticker{};
  const auto size = std::min((size_t)str->size(), TICKER_SIZE);
  std::memcpy(ticker.data(), str->data(), size);
  return ticker;
}

static inline String fbStringToString(const flatbuffers::String *str) {
  return String(str->c_str(), str->size());
}

OrderAction convert(gen::fbs::market::OrderAction action) {
  switch (action) {
  case gen::fbs::market::OrderAction::OrderAction_BUY:
    return OrderAction::Buy;
  case gen::fbs::market::OrderAction::OrderAction_SELL:
    return OrderAction::Sell;
  default:
    LOG_ERROR("Unknown OrderAction {}", (uint8_t)action);
    return OrderAction::Buy;
  }
}
gen::fbs::market::OrderAction convert(OrderAction action) {
  switch (action) {
  case OrderAction::Buy:
    return gen::fbs::market::OrderAction::OrderAction_BUY;
  case OrderAction::Sell:
    return gen::fbs::market::OrderAction::OrderAction_SELL;
  default:
    LOG_ERROR("Unknown OrderAction {}", (uint8_t)action);
    return gen::fbs::market::OrderAction::OrderAction_BUY;
  }
}

OrderState convert(gen::fbs::market::OrderState state) {
  switch (state) {
  case gen::fbs::market::OrderState::OrderState_Accepted:
    return OrderState::Accepted;
  case gen::fbs::market::OrderState::OrderState_Partial:
    return OrderState::Partial;
  case gen::fbs::market::OrderState::OrderState_Full:
    return OrderState::Full;
  default:
    LOG_ERROR("Unknown OrderState {}", (uint8_t)state);
    return static_cast<OrderState>(state);
  }
}
gen::fbs::market::OrderState convert(OrderState state) {
  switch (state) {
  case OrderState::Accepted:
    return gen::fbs::market::OrderState::OrderState_Accepted;
  case OrderState::Partial:
    return gen::fbs::market::OrderState::OrderState_Partial;
  case OrderState::Full:
    return gen::fbs::market::OrderState::OrderState_Full;
  default:
    LOG_ERROR("Unknown OrderState {}", (uint8_t)state);
    return static_cast<gen::fbs::market::OrderState>(state);
  }
}

TimestampType convert(gen::fbs::meta::TimestampType type) {
  switch (type) {
  case gen::fbs::meta::TimestampType::TimestampType_Created:
    return TimestampType::Created;
  case gen::fbs::meta::TimestampType::TimestampType_Received:
    return TimestampType::Received;
  case gen::fbs::meta::TimestampType::TimestampType_Fulfilled:
    return TimestampType::Fulfilled;
  case gen::fbs::meta::TimestampType::TimestampType_Notified:
    return TimestampType::Notified;
  case gen::fbs::meta::TimestampType::TimestampType_Closed:
    return TimestampType::Closed;
  default:
    LOG_ERROR("Unknown TimestampType {}", (uint8_t)type);
    return static_cast<TimestampType>(type);
  }
}
gen::fbs::meta::TimestampType convert(TimestampType type) {
  switch (type) {
  case TimestampType::Created:
    return gen::fbs::meta::TimestampType::TimestampType_Created;
  case TimestampType::Received:
    return gen::fbs::meta::TimestampType::TimestampType_Received;
  case TimestampType::Fulfilled:
    return gen::fbs::meta::TimestampType::TimestampType_Fulfilled;
  case TimestampType::Notified:
    return gen::fbs::meta::TimestampType::TimestampType_Notified;
  case TimestampType::Closed:
    return gen::fbs::meta::TimestampType::TimestampType_Closed;
  default:
    LOG_ERROR("Unknown TimestampType {}", (uint8_t)type);
    return static_cast<gen::fbs::meta::TimestampType>(type);
  }
}

} // namespace hft::serialization

#endif // HFT_COMMON_CONVERTER_HPP