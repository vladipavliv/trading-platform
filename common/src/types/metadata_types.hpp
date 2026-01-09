/**
 * @author Vladimir Pavliv
 * @date 2025-04-08
 */

#ifndef HFT_COMMON_METADATATYPES_HPP
#define HFT_COMMON_METADATATYPES_HPP

#include "domain_types.hpp"
#include "primitive_types.hpp"
#include "utils/string_utils.hpp"

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

struct RUsageSnapshot {
  // CPU time spent executing user code
  uint64_t userTimeNs{};
  // CPU time spent executing kernel code (system calls)
  uint64_t systemTimeNs{};
  // local context switches
  uint64_t volSwitches{};
  // forced context switches
  uint64_t involSwitches{};
  // Number of page faults that did not require disk access
  uint64_t softPageFaults{};
  // Number of page faults that required reading from disk (major page faults)
  uint64_t hardPageFaults{};
};

struct ProfilingData {
  CoreId coreId{};
  RUsageSnapshot ruSnapshot;
  uint64_t waitSpins{};
  uint64_t ftxWait{};
  uint64_t ftxWake{};
  uint64_t maxCall{};
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
  return std::format("OrderTimestamp: id:{} Created:{} Fulfilled:{} Notified:{}", event.orderId,
                     event.created, event.fulfilled, event.notified);
}

inline String toString(const RuntimeMetrics &event) {
  return std::format("RuntimeMetrics: Source:{} Ts:{} Rps:{} AngLat:{}", toString(event.source),
                     event.timeStamp, event.rps, utils::formatNs(event.avgLatencyUs));
}

inline String toString(const LogEntry &event) {
  return std::format("LogEntry: source:{} msg='{}' level:{}", toString(event.source), event.message,
                     event.level);
}

inline std::string toString(const RUsageSnapshot &ru) {
  return std::format("Rusg Usr:{} Sys:{} VolSw:{} InvolSw:{} SoftPgFlt:{} HardPgFlt:{}",
                     utils::formatNs(ru.userTimeNs), utils::formatNs(ru.systemTimeNs),
                     ru.volSwitches, ru.involSwitches, ru.softPageFaults, ru.hardPageFaults);
}

inline std::string toString(const ProfilingData &pd) {
  return std::format("Core:{} WaitSpins:{} FtxWait:{} FtxWake:{} MaxCall:{} {}", pd.coreId,
                     pd.waitSpins, pd.ftxWait, pd.ftxWake, utils::formatNs(pd.maxCall),
                     toString(pd.ruSnapshot));
}

} // namespace hft

#endif // HFT_COMMON_METADATATYPES_HPP
