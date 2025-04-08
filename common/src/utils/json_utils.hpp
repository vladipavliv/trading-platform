/**
 * @author Vladimir Pavliv
 * @date 2025-04-08
 */

#ifndef HFT_COMMON_JSONUTILS_HPP
#define HFT_COMMON_JSONUTILS_HPP

#include <nlohmann/json.hpp>

#include "market_types.hpp"
#include "string_utils.hpp"
#include "template_types.hpp"
#include "types.hpp"

namespace hft::utils {

String toJson(CRef<Order> order) {
  nlohmann::json js{{"traderId", order.traderId},
                    {"id", order.id},
                    {"timestamp", order.timestamp},
                    {"ticker", utils::toString(order.ticker)},
                    {"quantity", order.quantity},
                    {"price", order.price},
                    {"action", utils::toString(order.action)}};
  return js.dump();
}

String toJson(CRef<OrderStatus> status) {
  nlohmann::json js{{"traderId", status.traderId},
                    {"orderId", status.orderId},
                    {"timestamp", status.timestamp},
                    {"ticker", utils::toString(status.ticker)},
                    {"quantity", status.quantity},
                    {"fillPrice", status.fillPrice},
                    {"state", utils::toString(status.state)},
                    {"action", utils::toString(status.action)}};
  return js.dump();
}

} // namespace hft::utils

#endif // HFT_COMMON_JSONUTILS_HPP