/**
 * @author Vladimir Pavliv
 * @date 2025-08-09
 */

#ifndef HFT_SERVER_TYPECONVERTERS_HPP
#define HFT_SERVER_TYPECONVERTERS_HPP

#include "adapters/postgres/table_reader.hpp"
#include "adapters/postgres/table_writer.hpp"
#include "server_domain_types.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft::server {

inline adapters::TableWriter &operator<<(adapters::TableWriter &writer, CRef<ServerOrder> msg) {
  return writer << std::make_tuple(msg.clientId, msg.order.id, msg.order.created,
                                   utils::toString(msg.order.ticker), msg.order.quantity,
                                   msg.order.price, static_cast<uint16_t>(msg.order.action));
}
inline adapters::TableReader &operator>>(adapters::TableReader &reader, ServerOrder &msg) {
  size_t idx{0};
  msg.clientId = reader.get<ClientId>(idx++);
  msg.order.id = reader.get<OrderId>(idx++);
  msg.order.created = reader.get<Timestamp>(idx++);
  msg.order.ticker = utils::toTicker(reader.get<String>(idx++));
  msg.order.quantity = reader.get<Quantity>(idx++);
  msg.order.price = reader.get<Price>(idx++);
  msg.order.action = static_cast<OrderAction>(reader.get<uint16_t>(idx++));
  return reader;
}

} // namespace hft::server

#endif // HFT_SERVER_TYPECONVERTERS_HPP