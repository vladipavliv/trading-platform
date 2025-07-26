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
  OrderId orderId;
  Timestamp created;
  Timestamp fulfilled;
  Timestamp notified;
};

struct RuntimeMetrics {
  enum Source : uint8_t { Unknown, Client, Server };

  Source source;
  Timestamp timeStamp;
  size_t rps;
  size_t avgLatencyUs;
};

} // namespace hft

#endif // HFT_COMMON_METADATATYPES_HPP
