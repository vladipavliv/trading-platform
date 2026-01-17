/**
 * @author Vladimir Pavliv
 * @date 2026-01-13
 */

#ifndef HFT_COMMON_TELEMETRYADAPTER_HPP
#define HFT_COMMON_TELEMETRYADAPTER_HPP

#include "transport/shm/shm_ptr.hpp"
#include "transport/shm/shm_transport.hpp"
#include "types/container_types.hpp"
#include "types/telemetry_types.hpp"

namespace hft {

template <typename BusT>
class TelemetryAdapter {
public:
  TelemetryAdapter(BusT &bus, bool producer) : bus_{bus}, producer_{producer}, transport_{init()} {}

  void start() {
    LOG_DEBUG_SYSTEM("TelemetryAdapter start");
    if (producer_) {
      using SelfT = TelemetryAdapter<BusT>;
      bus_.template subscribe<TelemetryMsg>(
          CRefHandler<TelemetryMsg>::template bind<SelfT, &SelfT::post>(this));
    } else {
      ByteSpan span(reinterpret_cast<uint8_t *>(&msg_), sizeof(TelemetryMsg));
      transport_.asyncRx(span, [this](IoResult res, size_t size) { bus_.post(msg_); });
    }
  }

  void close() { transport_.close(); }

private:
  ShmTransport init() {
    const auto name = Config::get<String>("shm.shm_telemetry");
    if (producer_) {
      return ShmTransport::makeWriter(name);
    } else {
      return ShmTransport::makeReader(name);
    }
  }

  void post(CRef<TelemetryMsg> msg) {
    auto *ptr = reinterpret_cast<const uint8_t *>(&msg);
    CByteSpan span(ptr, sizeof(TelemetryMsg));
    transport_.asyncTx(span, [](IoResult, size_t) {}, 0);
  }

private:
  BusT &bus_;
  const bool producer_;

  ShmTransport transport_;
  TelemetryMsg msg_;
};
} // namespace hft

#endif // HFT_COMMON_TELEMETRYADAPTER_HPP
