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
  OrderId externalOId;
  SystemOrderId systemOId;
  BookOrderId bookOId;

  ClientId clientId;
  Ticker ticker;
};
} // namespace hft::server

#endif // HFT_SERVER_ORDERRECORD_HPP