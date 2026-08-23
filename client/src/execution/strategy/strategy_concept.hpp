/**
 * @author Vladimir Pavliv
 * @date 2026-08-23
 */

#ifndef HFT_COMMON_STRATEGY_CONCEPT_HPP
#define HFT_COMMON_STRATEGY_CONCEPT_HPP

#include <concepts>
#include <utility>

#include "domain_types.hpp"

namespace hft::client {

/**
 * @brief Concept for a client trade strategy
 */
template <typename Strategy>
concept TradeStrategy = requires(Strategy &s) {
  { s.execute() } -> std::same_as<void>;
};

} // namespace hft::client

#endif // HFT_COMMON_STRATEGY_CONCEPT_HPP
