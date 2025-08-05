/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_SERIALIZATION_FBSCONVERTER_HPP
#define HFT_COMMON_SERIALIZATION_FBSCONVERTER_HPP

#include "domain_types.hpp"
#include "gen/fbs/cpp/domain_messages_generated.h"
#include "logging.hpp"
#include "types.hpp"

namespace hft::serialization {

inline Ticker fbStringToTicker(const flatbuffers::String *str) {
  Ticker ticker{};
  const auto size = std::min((size_t)str->size(), TICKER_SIZE);
  std::memcpy(ticker.data(), str->data(), size);
  return ticker;
}

inline String fbStringToString(const flatbuffers::String *str) {
  return String(str->c_str(), str->size());
}

inline OrderAction convert(gen::fbs::domain::OrderAction action) {
  switch (action) {
  case gen::fbs::domain::OrderAction::OrderAction_BUY:
    return OrderAction::Buy;
  case gen::fbs::domain::OrderAction::OrderAction_SELL:
    return OrderAction::Sell;
  default:
    LOG_ERROR("Unknown OrderAction {}", (uint8_t)action);
    return OrderAction::Buy;
  }
}
inline gen::fbs::domain::OrderAction convert(OrderAction action) {
  switch (action) {
  case OrderAction::Buy:
    return gen::fbs::domain::OrderAction::OrderAction_BUY;
  case OrderAction::Sell:
    return gen::fbs::domain::OrderAction::OrderAction_SELL;
  default:
    LOG_ERROR("Unknown OrderAction {}", (uint8_t)action);
    return gen::fbs::domain::OrderAction::OrderAction_BUY;
  }
}

inline OrderState convert(gen::fbs::domain::OrderState state) {
  switch (state) {
  case gen::fbs::domain::OrderState::OrderState_Accepted:
    return OrderState::Accepted;
  case gen::fbs::domain::OrderState::OrderState_Rejected:
    return OrderState::Rejected;
  case gen::fbs::domain::OrderState::OrderState_Partial:
    return OrderState::Partial;
  case gen::fbs::domain::OrderState::OrderState_Full:
    return OrderState::Full;
  default:
    LOG_ERROR("Unknown OrderState {}", (uint8_t)state);
    return static_cast<OrderState>(state);
  }
}
inline gen::fbs::domain::OrderState convert(OrderState state) {
  switch (state) {
  case OrderState::Accepted:
    return gen::fbs::domain::OrderState::OrderState_Accepted;
  case OrderState::Rejected:
    return gen::fbs::domain::OrderState::OrderState_Rejected;
  case OrderState::Partial:
    return gen::fbs::domain::OrderState::OrderState_Partial;
  case OrderState::Full:
    return gen::fbs::domain::OrderState::OrderState_Full;
  default:
    LOG_ERROR("Unknown OrderState {}", (uint8_t)state);
    return static_cast<gen::fbs::domain::OrderState>(state);
  }
}

} // namespace hft::serialization

#endif // HFT_COMMON_SERIALIZATION_FBSCONVERTER_HPP
