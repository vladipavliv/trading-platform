/**
 * @author Vladimir Pavliv
 * @date 2025-08-09
 */

#ifndef HFT_COMMON_SERIALIZATION_SBECONVERTER_HPP
#define HFT_COMMON_SERIALIZATION_SBECONVERTER_HPP

#include "domain_types.hpp"
#include "logging.hpp"
#include "types.hpp"

#include "gen/sbe/cpp/hft_serialization_gen_sbe_domain/OrderAction.h"
#include "gen/sbe/cpp/hft_serialization_gen_sbe_domain/OrderState.h"

namespace hft::serialization::sbe {

OrderAction convert(gen::sbe::domain::OrderAction::Value action) {
  switch (action) {
  case gen::sbe::domain::OrderAction::Value::BUY:
    return OrderAction::Buy;
  case gen::sbe::domain::OrderAction::Value::SELL:
    return OrderAction::Sell;
  default:
    throw std::runtime_error("Unknown gen::sbe::domain::OrderAction::Value type");
  }
}

gen::sbe::domain::OrderAction::Value convert(OrderAction action) {
  switch (action) {
  case OrderAction::Buy:
    return gen::sbe::domain::OrderAction::Value::BUY;
  case OrderAction::Sell:
    return gen::sbe::domain::OrderAction::Value::SELL;
  default:
    throw std::runtime_error("OrderAction type");
  }
}

OrderState convert(gen::sbe::domain::OrderState::Value action) {
  switch (action) {
  case gen::sbe::domain::OrderState::Value::Accepted:
    return OrderState::Accepted;
  case gen::sbe::domain::OrderState::Value::Rejected:
    return OrderState::Rejected;
  case gen::sbe::domain::OrderState::Value::Partial:
    return OrderState::Partial;
  case gen::sbe::domain::OrderState::Value::Full:
    return OrderState::Full;
  default:
    throw std::runtime_error("Unknown gen::sbe::domain::OrderState::Value type");
  }
}

gen::sbe::domain::OrderState::Value convert(OrderState action) {
  switch (action) {
  case OrderState::Accepted:
    return gen::sbe::domain::OrderState::Value::Accepted;
  case OrderState::Rejected:
    return gen::sbe::domain::OrderState::Value::Rejected;
  case OrderState::Partial:
    return gen::sbe::domain::OrderState::Value::Partial;
  case OrderState::Full:
    return gen::sbe::domain::OrderState::Value::Full;
  default:
    throw std::runtime_error("OrderState type");
  }
}

} // namespace hft::serialization::sbe

#endif // HFT_COMMON_SERIALIZATION_SBECONVERTER_HPP