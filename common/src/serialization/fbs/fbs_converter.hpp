/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_SERIALIZATION_FBSCONVERTER_HPP
#define HFT_COMMON_SERIALIZATION_FBSCONVERTER_HPP

#include "domain_types.hpp"
#include "fbs/cpp/domain_messages_generated.h"
#include "logging.hpp"
#include "types.hpp"

namespace hft::serialization::fbs {

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
    throw std::runtime_error(
        std::format("Unknown gen::fbs::domain::OrderAction {}", (uint8_t)action));
  }
}
inline gen::fbs::domain::OrderAction convert(OrderAction action) {
  switch (action) {
  case OrderAction::Buy:
    return gen::fbs::domain::OrderAction::OrderAction_BUY;
  case OrderAction::Sell:
    return gen::fbs::domain::OrderAction::OrderAction_SELL;
  default:
    throw std::runtime_error(std::format("Unknown OrderAction {}", (uint8_t)action));
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
    throw std::runtime_error(
        std::format("Unknown gen::fbs::domain::OrderState {}", (uint8_t)state));
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
    throw std::runtime_error(std::format("Unknown OrderState {}", (uint8_t)state));
  }
}

} // namespace hft::serialization::fbs

#endif // HFT_COMMON_SERIALIZATION_FBSCONVERTER_HPP
