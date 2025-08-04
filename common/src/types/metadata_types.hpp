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

enum MetadataSource : uint8_t { Unknown, Client, Server };

struct RuntimeMetrics {
  MetadataSource source;
  Timestamp timeStamp;
  size_t rps;
  size_t avgLatencyUs;
};

struct LogEntry {
  MetadataSource source;
  String message;
  String level;
};

} // namespace hft

#endif // HFT_COMMON_METADATATYPES_HPP
