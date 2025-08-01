/**
 * @author Vladimir Pavliv
 * @date 2025-01-08
 */

#ifndef HFT_SERVER_DBTYPEMAPPER_HPP
#define HFT_SERVER_DBTYPEMAPPER_HPP

#include "server_types.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft::server {

template <typename T>
struct DbTypeMapper;

template <typename MessageType>
concept HasTable = requires {
  { DbTypeMapper<MessageType>::table() } -> std::same_as<String>;
};

template <>
struct DbTypeMapper<Order> {
  static auto toRow(CRef<Order> order) -> std::vector<String> {
    return {std::to_string(order.id),      std::to_string(order.created),
            utils::toString(order.ticker), std::to_string(order.quantity),
            std::to_string(order.price),   std::to_string(static_cast<uint8_t>(order.action))};
  }

  static auto fromRow(CRef<std::vector<String>> row) -> Order {
    if (row.size() < 6) {
      throw std::runtime_error("Invalid row size for Order");
    }
    Order order;
    order.id = static_cast<OrderId>(std::stoll(row[0]));
    order.created = static_cast<Timestamp>(std::stoll(row[1]));
    std::copy_n(row[2].begin(), row[2].size(), order.ticker.begin());
    order.quantity = static_cast<Quantity>(std::stoll(row[3]));
    order.price = static_cast<Price>(std::stod(row[4]));
    order.action = static_cast<OrderAction>(std::stoi(row[5]));
    return order;
  }
};

template <>
struct DbTypeMapper<ServerOrder> {
  static auto table() -> String { return "orders"; }

  static auto toRow(CRef<ServerOrder> order) -> std::vector<String> {
    const auto orderRow = DbTypeMapper<Order>::toRow(order.order);
    std::vector<String> row;
    row.reserve(orderRow.size() + 1);
    row.push_back(std::to_string(order.clientId));
    row.insert(row.end(), orderRow.begin(), orderRow.end());
    return row;
  }

  static auto fromRow(CRef<std::vector<String>> row) -> ServerOrder {
    if (row.size() < 7) {
      throw std::runtime_error("Invalid row size for ServerOrder");
    }
    ServerOrder order;
    order.clientId = static_cast<ClientId>(std::stoll(row[0]));
    const std::vector<String> orderRow(row.begin() + 1, row.end());
    order.order = DbTypeMapper<Order>::fromRow(orderRow);
    return order;
  }
};

} // namespace hft::server

#endif // HFT_SERVER_DBTYPEMAPPER_HPP