/**
 * @author Vladimir Pavliv
 * @date 2026-08-23
 */

#ifndef HFT_COMMON_STRATEGIES_HPP
#define HFT_COMMON_STRATEGIES_HPP

#include <tuple>

#include "random_strategy.hpp"
#include "ttl_cancel_strategy.hpp"

namespace hft::client {

using Strategies = std::tuple<RandomStrategy, TtlCancelStrategy>;

} // namespace hft::client

#endif // HFT_COMMON_STRATEGIES_HPP
