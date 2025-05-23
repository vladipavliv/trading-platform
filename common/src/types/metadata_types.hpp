/**
 * @author Vladimir Pavliv
 * @date 2025-04-08
 */

#ifndef HFT_COMMON_METADATATYPES_HPP
#define HFT_COMMON_METADATATYPES_HPP

#include "domain_types.hpp"
#include "types.hpp"

namespace hft {

struct OrderTimestamp {
  const OrderId orderId;
  const Timestamp created;
  const Timestamp fulfilled;
  const Timestamp notified;
};

} // namespace hft

#endif // HFT_COMMON_METADATATYPES_HPP
