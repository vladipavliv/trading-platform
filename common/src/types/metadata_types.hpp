/**
 * @author Vladimir Pavliv
 * @date 2025-04-08
 */

#ifndef HFT_COMMON_METADATATYPES_HPP
#define HFT_COMMON_METADATATYPES_HPP

#include "domain_types.hpp"
#include "primitive_types.hpp"

namespace hft {

enum MetadataSource : uint8_t { Unknown, Client, Server };

struct OrderTimestamp {
  OrderId orderId;
  Timestamp created;
  Timestamp fulfilled;
  Timestamp notified;
};

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

struct CtxSwitches {
  uint32_t inv{0};
  uint32_t vol{0};
};

struct ThreadCounters {
  CoreId id{};
  CtxSwitches ctxSwitches;
  uint64_t pause{0};
  uint64_t futexWait{0};
  uint64_t futexWake{0};
  uint64_t maxDrain{0};
};

inline String toString(const MetadataSource &event) {
  switch (event) {
  case MetadataSource::Client:
    return "Client";
  case MetadataSource::Server:
    return "Server";
  default:
    return "Unknown";
  }
}

inline String toString(const OrderTimestamp &event) {
  return std::format("OrderTimestamp: id={} created={} fulfilled={} notified={}", event.orderId,
                     event.created, event.fulfilled, event.notified);
}

inline String toString(const RuntimeMetrics &event) {
  return std::format("RuntimeMetrics: source={} ts={} rps={} avg_lat={}us", toString(event.source),
                     event.timeStamp, event.rps, event.avgLatencyUs);
}

inline String toString(const LogEntry &event) {
  return std::format("LogEntry: source={} msg='{}' level={}", toString(event.source), event.message,
                     event.level);
}

} // namespace hft

#endif // HFT_COMMON_METADATATYPES_HPP
