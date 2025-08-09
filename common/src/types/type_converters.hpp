/**
 * @author Vladimir Pavliv
 * @date 2025-08-09
 */

#ifndef HFT_COMMON_TYPECONVERTERS_HPP
#define HFT_COMMON_TYPECONVERTERS_HPP

#include "adapters/postgres/table_reader.hpp"
#include "adapters/postgres/table_writer.hpp"
#include "domain_types.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft {

inline adapters::TableWriter &operator<<(adapters::TableWriter &writer, CRef<LoginRequest> msg) {
  return writer << std::make_tuple(msg.name, msg.password);
}
inline adapters::TableReader &operator>>(adapters::TableReader &reader, LoginRequest &msg) {
  size_t idx{0};
  msg.name = reader.get<String>(idx++);
  msg.name = reader.get<String>(idx++);
  return reader;
}

inline adapters::TableWriter &operator<<(adapters::TableWriter &writer, CRef<LoginResponse> msg) {
  return writer << std::make_tuple(msg.token, msg.ok, msg.error);
}
inline adapters::TableReader &operator>>(adapters::TableReader &reader, LoginResponse &msg) {
  size_t idx{0};
  msg.token = reader.get<Token>(idx++);
  msg.ok = reader.get<bool>(idx++);
  msg.error = reader.get<String>(idx++);
  return reader;
}

inline adapters::TableWriter &operator<<(adapters::TableWriter &writer,
                                         CRef<TokenBindRequest> msg) {
  return writer << std::make_tuple(msg.token);
}
inline adapters::TableReader &operator>>(adapters::TableReader &reader, TokenBindRequest &msg) {
  size_t idx{0};
  msg.token = reader.get<Token>(idx);
  return reader;
}

inline adapters::TableWriter &operator<<(adapters::TableWriter &writer, CRef<Order> msg) {
  return writer << std::make_tuple(msg.id, msg.created, utils::toString(msg.ticker), msg.quantity,
                                   msg.price, static_cast<uint16_t>(msg.action));
}
inline adapters::TableReader &operator>>(adapters::TableReader &reader, Order &msg) {
  size_t idx{0};
  msg.id = reader.get<OrderId>(idx++);
  msg.created = reader.get<Timestamp>(idx++);
  msg.ticker = utils::toTicker(reader.get<String>(idx++));
  msg.quantity = reader.get<Quantity>(idx++);
  msg.price = reader.get<Price>(idx++);
  msg.action = static_cast<OrderAction>(reader.get<uint16_t>(idx++));
  return reader;
}

inline adapters::TableWriter &operator<<(adapters::TableWriter &writer, CRef<OrderStatus> msg) {
  return writer << std::make_tuple(msg.orderId, msg.timeStamp, msg.quantity, msg.fillPrice,
                                   static_cast<uint16_t>(msg.state));
}
inline adapters::TableReader &operator>>(adapters::TableReader &reader, OrderStatus &msg) {
  size_t idx{0};
  msg.orderId = reader.get<OrderId>(idx++);
  msg.timeStamp = reader.get<Timestamp>(idx++);
  msg.quantity = reader.get<Quantity>(idx++);
  msg.fillPrice = reader.get<Price>(idx++);
  msg.state = static_cast<OrderState>(reader.get<uint16_t>(idx++));
  return reader;
}

} // namespace hft

#endif // HFT_SERVER_TYPECONVERTERS_HPP