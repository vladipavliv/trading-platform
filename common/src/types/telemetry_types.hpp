/**
 * @author Vladimir Pavliv
 * @date 2025-04-08
 */

#ifndef HFT_COMMON_TELEMETRYTYPES_HPP
#define HFT_COMMON_TELEMETRYTYPES_HPP

#include "domain_types.hpp"
#include "primitive_types.hpp"
#include "utils/string_utils.hpp"

namespace hft {

enum class Source : uint8_t { Server, Client };

enum class TelemetryType : uint8_t {
  Startup = 0,      // One-time: PID and Core mapping
  OrderLatency = 1, // The hot path "OrderTimestamp"
  Runtime = 2,      // RPS and AvgLat
  Profiling = 3,    // Spins, Futex, MaxCall
  Log = 4           // Error strings
};

#pragma pack(push, 1)
struct TelemetryMsg {
  // --- Header (4 bytes) ---
  TelemetryType type;
  Source source;
  uint16_t componentId;

  // --- Payload (48 bytes) ---
  union {
    struct {
      uint32_t pid;
      uint32_t coreId;
      char buildInfo[40];
    } init;

    struct {
      OrderId id;
      Timestamp created;
      Timestamp fulfilled;
      Timestamp notified;
      uint64_t ordersPlaced;
      uint64_t ordersFulfilled;
    } order;

    struct {
      Timestamp ts;
      uint64_t rps;
      uint64_t avgLatNs;
    } metrics;

    struct {
      uint64_t waitSpins;
      uint64_t ftxWait;
      uint64_t ftxWake;
      uint64_t maxCallNs;
    } prof;

    struct {
      uint8_t level;
      char msg[47];
    } log;

    uint8_t raw[48];
  } data;
};
#pragma pack(pop)

static_assert(sizeof(TelemetryMsg) == 52, "TelemetryMsg must be exactly 52 bytes");

inline std::string toString(const TelemetryMsg &msg) {
  std::string header =
      std::format("Source:{} | ", (msg.source == Source::Server ? "Server" : "Client"));

  switch (msg.type) {
  case TelemetryType::Startup:
    return header + std::format("Startup: Pid:{} Core:{} Build:{}", msg.data.init.pid,
                                msg.data.init.coreId, utils::fromArray(msg.data.init.buildInfo));

  case TelemetryType::OrderLatency:
    return header +
           std::format("Order: ID:{} Created:{} Fulfilled:{} Notified:{} Placed:{} Fulfilled:{}",
                       msg.data.order.id, msg.data.order.created, msg.data.order.fulfilled,
                       msg.data.order.notified, msg.data.order.ordersPlaced,
                       msg.data.order.ordersFulfilled);

  case TelemetryType::Runtime:
    return header + std::format("Metrics: TS:{} RPS:{} AvgLat:{}ns", msg.data.metrics.ts,
                                msg.data.metrics.rps, msg.data.metrics.avgLatNs);

  case TelemetryType::Profiling:
    return header + std::format("Profiling: Spins:{} FtxWait:{} FtxWake:{} MaxCall:{}ns",
                                msg.data.prof.waitSpins, msg.data.prof.ftxWait,
                                msg.data.prof.ftxWake, msg.data.prof.maxCallNs);

  case TelemetryType::Log:
    return header + std::format("LOG: Level:{} Msg:{}", static_cast<int>(msg.data.log.level),
                                utils::fromArray(msg.data.log.msg));

  default:
    return header + "Unknown Telemetry Type";
  }
}

} // namespace hft

#endif // HFT_COMMON_TELEMETRYTYPES_HPP
