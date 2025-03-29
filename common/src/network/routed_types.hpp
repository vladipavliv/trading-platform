/**
 * @author Vladimir Pavliv
 * @date 2025-03-23
 */

#ifndef HFT_COMMON_ROUTEDTYPES_HPP
#define HFT_COMMON_ROUTEDTYPES_HPP

#include <tuple>

#include "market_types.hpp"
#include "template_types.hpp"

namespace hft {

using RoutedTypes = std::tuple<Order, OrderStatus, TickerPrice>;

template <typename Type>
concept RoutedType = IsTypeInTuple<Type, RoutedTypes>;

enum class RoutedMessageType : uint8_t { Order, OrderStatus, TickerPrice };

} // namespace hft

#endif // HFT_COMMON_ROUTEDTYPES_HPP