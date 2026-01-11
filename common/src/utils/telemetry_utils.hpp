/**
 * @author Vladimir Pavliv
 * @date 2026-01-11
 */

#ifndef HFT_COMMON_TELEMETRYUTILS_HPP
#define HFT_COMMON_TELEMETRYUTILS_HPP

#include "primitive_types.hpp"
#include "telemetry_types.hpp"

namespace hft::utils {

inline TelemetryMsg createBaseMsg(TelemetryType type, Source source, uint16_t componentId) {
  TelemetryMsg msg;
  msg.type = type;
  msg.source = source;
  msg.componentId = componentId;
  return msg;
}

inline TelemetryMsg createStartupMsg(Source source, uint16_t compId, uint32_t pid, uint32_t coreId,
                                     const char *build) {
  auto msg = createBaseMsg(TelemetryType::Startup, source, compId);
  msg.data.init.pid = pid;
  msg.data.init.coreId = coreId;
  std::strncpy(msg.data.init.buildInfo, build, sizeof(msg.data.init.buildInfo) - 1);
  msg.data.init.buildInfo[sizeof(msg.data.init.buildInfo) - 1] = '\0';
  return msg;
}

inline TelemetryMsg createOrderLatencyMsg(Source source, uint16_t compId, OrderId id,
                                          uint64_t start, uint64_t fulfilled, uint64_t notified) {
  auto msg = createBaseMsg(TelemetryType::OrderLatency, source, compId);
  msg.data.order.id = id;
  msg.data.order.created = start;
  msg.data.order.fulfilled = fulfilled;
  msg.data.order.notified = notified;
  return msg;
}

inline TelemetryMsg createRuntimeMsg(Source source, uint16_t compId, Timestamp ts, uint64_t rps,
                                     uint64_t avgLatNs) {
  auto msg = createBaseMsg(TelemetryType::Runtime, source, compId);
  msg.data.metrics.ts = ts;
  msg.data.metrics.rps = rps;
  msg.data.metrics.avgLatNs = avgLatNs;
  return msg;
}

inline TelemetryMsg createProfilingMsg(Source source, uint16_t compId, uint64_t spins,
                                       uint64_t wait, uint64_t wake, uint64_t maxCall) {
  auto msg = createBaseMsg(TelemetryType::Profiling, source, compId);
  msg.data.prof.waitSpins = spins;
  msg.data.prof.ftxWait = wait;
  msg.data.prof.ftxWake = wake;
  msg.data.prof.maxCallNs = maxCall;
  return msg;
}

inline TelemetryMsg createLogMsg(Source source, uint16_t compId, uint8_t level, const char *text) {
  auto msg = createBaseMsg(TelemetryType::Log, source, compId);
  msg.data.log.level = level;
  std::strncpy(msg.data.log.msg, text, sizeof(msg.data.log.msg) - 1);
  msg.data.log.msg[sizeof(msg.data.log.msg) - 1] = '\0';
  return msg;
}

} // namespace hft::utils

#endif // HFT_COMMON_TELEMETRYUTILS_HPP