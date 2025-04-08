/**
 * @author Vladimir Pavliv
 * @date 2025-04-08
 */

#ifndef HFT_COMMON_JSONPARSER_HPP
#define HFT_COMMON_JSONPARSER_HPP

#include <nlohmann/json.hpp>

#include "market_types.hpp"
#include "string_utils.hpp"
#include "template_types.hpp"
#include "types.hpp"

namespace hft::json {

using SupportedTypes = std::tuple<Order, OrderStatus, TickerPrice>;

template <typename EventType>
concept Jsonable = IsTypeInTuple<EventType, SupportedTypes>;

/**
 * @brief Parses stuff to/from json via nlohmann
 * @details Plans changed and its no longer in use. Keeping just in case
 */
class JsonParser {
  using NloJson = nlohmann::json;

public:
  template <Jsonable EventType>
  static String toJson(CRef<EventType> event) {
    if constexpr (std::is_same_v<EventType, Order>) {
      return NloJson{{"traderId", obj.traderId},
                     {"id", obj.id},
                     {"timestamp", obj.timestamp},
                     {"ticker", utils::toString(obj.ticker)},
                     {"quantity", obj.quantity},
                     {"price", obj.price},
                     {"action", utils::toString(obj.action)}}
          .dump();
    } else if constexpr (std::is_same_v<EventType, OrderStatus>) {
      return NloJson{{"traderId", obj.traderId},
                     {"orderId", obj.orderId},
                     {"timestamp", obj.timestamp},
                     {"ticker", utils::toString(obj.ticker)},
                     {"quantity", obj.quantity},
                     {"fillPrice", obj.fillPrice},
                     {"state", utils::toString(obj.state)},
                     {"action", utils::toString(obj.action)}}
          .dump();
    }
  }
  template <Jsonable EventType>
  static EventType fromJson(CRef<String> json) {
    NloJson js = NloJson::parse(json);
    if constexpr (std::is_same_v<EventType, Order>) {
      Order order;
      order.traderId = js["traderId"];
      order.id = js["id"];
      order.timestamp = js["timestamp"];
      order.ticker = utils::fromString(js["ticker"]);
      order.quantity = js["quantity"];
      order.price = js["price"];
      order.action = utils::fromString(js["action"]);
      return order;
    } else if constexpr (std::is_same_v<EventType, OrderStatus>) {
      OrderStatus status;
      status.traderId = js["traderId"];
      status.orderId = js["orderId"];
      status.timestamp = js["timestamp"];
      status.ticker = utils::fromString(js["ticker"]);
      status.quantity = js["quantity"];
      status.fillPrice = js["fillPrice"];
      status.state = utils::fromString(js["state"]);
      status.action = utils::fromString(js["action"]);
      return status;
    }
  }
};
} // namespace hft::json

#endif // HFT_COMMON_JSONPARSER_HPP