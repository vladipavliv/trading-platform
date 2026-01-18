/**
 * @author Vladimir Pavliv
 * @date 2026-01-14
 */

#ifndef HFT_SERVER_ORDERRECORD_HPP
#define HFT_SERVER_ORDERRECORD_HPP

#include "domain_types.hpp"
#include "schema.hpp"

namespace hft::server {
struct OrderRecord {
  Timestamp created;
  SystemOrderId id;
  BookOrderId bookOId;
  ClientId clientId;
  OrderId externalId;
};
} // namespace hft::server

#endif // HFT_SERVER_ORDERRECORD_HPP