/**
 * @author Vladimir Pavliv
 * @date 2026-01-14
 */

#ifndef HFT_SERVER_ORDERRECORD_HPP
#define HFT_SERVER_ORDERRECORD_HPP

#include "domain_types.hpp"
#include "internal_id.hpp"

namespace hft::server {
struct OrderRecord {
  Timestamp created;
  InternalOrderId id;
  ClientId clientId;
};
} // namespace hft::server

#endif // HFT_SERVER_ORDERRECORD_HPP