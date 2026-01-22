/**
 * @author Vladimir Pavliv
 * @date 2026-01-13
 */

#ifndef HFT_COMMON_TELEMETRYADAPTER_HPP
#define HFT_COMMON_TELEMETRYADAPTER_HPP

#include "transport/shm/shm_ptr.hpp"
#include "transport/shm/shm_reactor.hpp"
#include "transport/shm/shm_transport.hpp"
#include "types/container_types.hpp"
#include "types/telemetry_types.hpp"
#include "utils/handler.hpp"

namespace hft {

template <typename BusT>
class TelemetryAdapter {
  using SelfT = TelemetryAdapter<BusT>;

public:
  TelemetryAdapter(BusT &bus, const Config &cfg, bool producer)
      : bus_{bus}, config_{cfg}, producer_{producer}, transport_{init()} {}

  void start() {
    LOG_DEBUG_SYSTEM("TelemetryAdapter start");
    if (producer_) {
      bus_.subscribe(CRefHandler<TelemetryMsg>::bind<SelfT, &SelfT::post>(this));
    } else {
      ByteSpan span(reinterpret_cast<uint8_t *>(&msg_), sizeof(TelemetryMsg));
      transport_.asyncRx(span, CRefHandler<IoResult>::bind<SelfT, &SelfT::post>(this));
    }
  }

  void close() { transport_.close(); }

private:
  ShmTransport init() {
    const auto name = config_.get<String>("shm.shm_telemetry");
    if (producer_) {
      return ShmTransport::makeWriter(name);
    } else {
      return ShmTransport::makeReader(name);
    }
  }

  void post(CRef<TelemetryMsg> msg) {
    auto *ptr = reinterpret_cast<const uint8_t *>(&msg);
    CByteSpan span(ptr, sizeof(TelemetryMsg));
    if (!transport_.syncTx(span)) {
      LOG_ERROR("Failed to write telemetry");
    }
  }

  void post(CRef<IoResult> res) {
    if (!res) {
      LOG_ERROR_SYSTEM("Failed to read from shm");
    } else {
      bus_.post(msg_);
    }
  }

private:
  BusT &bus_;
  const Config &config_;

  const bool producer_;
  TelemetryMsg msg_;

  ShmTransport transport_;
};
} // namespace hft

#endif // HFT_COMMON_TELEMETRYADAPTER_HPP
