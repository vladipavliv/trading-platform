/**
 * @author Vladimir Pavliv
 * @date 2026-01-13
 */

#ifndef HFT_COMMON_TELEMETRYADAPTER_HPP
#define HFT_COMMON_TELEMETRYADAPTER_HPP

#include "transport/shm/shm_layout.hpp"
#include "transport/shm/shm_manager.hpp"
#include "transport/shm/shm_transport.hpp"
#include "types/telemetry_types.hpp"

namespace hft {

template <typename BusT>
class TelemetryAdapter {
public:
  TelemetryAdapter(BusT &bus, bool producer)
      : bus_{bus}, layout_{ShmManager::layout()}, transport_{init(producer)}, producer_{producer} {
    if (producer_) {
      bus_.template subscribe<TelemetryMsg>([this](CRef<TelemetryMsg> msg) {
        auto *ptr = reinterpret_cast<const uint8_t *>(&msg);
        CByteSpan span(ptr, sizeof(TelemetryMsg));
        transport_.asyncTx(span, [](IoResult, size_t) {}, 0);
      });
    } else {
      ByteSpan span(reinterpret_cast<uint8_t *>(&msg_), sizeof(TelemetryMsg));
      transport_.asyncRx(span, [this](IoResult res, size_t size) { bus_.post(msg_); });
    }
  }

  void close() { transport_.close(); }

private:
  ShmTransport init(bool producer) {
    if (producer) {
      return ShmTransport::makeWriter(layout_.telemetry);
    } else {
      return ShmTransport::makeReader(layout_.telemetry, ErrorBus{bus_.systemBus});
    }
  }

private:
  BusT &bus_;
  ShmLayout &layout_;

  TelemetryMsg msg_;
  ShmTransport transport_;

  const bool producer_;
};
} // namespace hft

#endif // HFT_COMMON_TELEMETRYADAPTER_HPP
