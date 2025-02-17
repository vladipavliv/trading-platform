/**
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_CONVERTER_HPP
#define HFT_COMMON_CONVERTER_HPP

#include <spdlog/spdlog.h>

#include "gen/marketdata_generated.h"
#include "market_types.hpp"
#include "types.hpp"

namespace hft::serialization {

static inline Ticker toString(const flatbuffers::String *str) {
  Ticker ticker{};
  std::strncpy(ticker.data(), str->data(),
               (str->size() >= ticker.size()) ? ticker.size() : str->size());
  return ticker;
}

OrderAction convert(gen::fbs::OrderAction action) {
  switch (action) {
  case gen::fbs::OrderAction::OrderAction_BUY:
    return OrderAction::Buy;
  case gen::fbs::OrderAction::OrderAction_SELL:
    return OrderAction::Sell;
  default:
    spdlog::error("Unknown OrderAction {}", (uint8_t)action);
    return OrderAction::Buy;
  }
}

gen::fbs::OrderAction convert(OrderAction action) {
  switch (action) {
  case OrderAction::Buy:
    return gen::fbs::OrderAction::OrderAction_BUY;
  case OrderAction::Sell:
    return gen::fbs::OrderAction::OrderAction_SELL;
  default:
    spdlog::error("Unknown OrderAction {}", (uint8_t)action);
    return gen::fbs::OrderAction::OrderAction_BUY;
  }
}

OrderState convert(gen::fbs::OrderState state) { return static_cast<OrderState>(state); }

gen::fbs::OrderState convert(OrderState state) { return static_cast<gen::fbs::OrderState>(state); }

} // namespace hft::serialization

#endif // HFT_COMMON_CONVERTER_HPP