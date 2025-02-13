/**
 * @file
 * @brief
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
  case gen::fbs::OrderAction::OrderAction_LIMIT:
    return OrderAction::Limit;
  case gen::fbs::OrderAction::OrderAction_MARKET:
    return OrderAction::Market;
  default:
    spdlog::error("Unknown order action");
    throw std::runtime_error("Unknown order action");
  }
}

gen::fbs::OrderAction convert(OrderAction action) {
  switch (action) {
  case OrderAction::Buy:
    return gen::fbs::OrderAction::OrderAction_BUY;
  case OrderAction::Sell:
    return gen::fbs::OrderAction::OrderAction_SELL;
  case OrderAction::Limit:
    return gen::fbs::OrderAction::OrderAction_LIMIT;
  case OrderAction::Market:
    return gen::fbs::OrderAction::OrderAction_MARKET;
  default:
    spdlog::error("Unknown order action");
    throw std::runtime_error("Unknown order action");
  }
}

FulfillmentState convert(gen::fbs::FulfillmentState state) {
  switch (state) {
  case gen::fbs::FulfillmentState::FulfillmentState_FULL:
    return FulfillmentState::Full;
  case gen::fbs::FulfillmentState::FulfillmentState_PARTIAL:
    return FulfillmentState::Partial;
  default:
    spdlog::error("Unknown fulfillment state");
    throw std::runtime_error("Unknown fulfillment state");
  }
}

gen::fbs::FulfillmentState convert(FulfillmentState state) {
  switch (state) {
  case FulfillmentState::Full:
    return gen::fbs::FulfillmentState::FulfillmentState_FULL;
  case FulfillmentState::Partial:
    return gen::fbs::FulfillmentState::FulfillmentState_PARTIAL;
  default:
    spdlog::error("Unknown fulfillment state");
    throw std::runtime_error("Unknown fulfillment state");
  }
}

} // namespace hft::serialization

#endif // HFT_COMMON_CONVERTER_HPP