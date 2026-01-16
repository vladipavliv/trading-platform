/**
 * @author Vladimir Pavliv
 * @date 2026-01-13
 */

#ifndef HFT_COMMON_DUMMYTELEMETRYADAPTER_HPP
#define HFT_COMMON_DUMMYTELEMETRYADAPTER_HPP

namespace hft {

template <typename BusT>
class DummyTelemetryAdapter {
public:
  DummyTelemetryAdapter(BusT &bus, bool producer) {}

  void start() {}

  void close() {}
};
} // namespace hft

#endif // HFT_COMMON_DUMMYTELEMETRYADAPTER_HPP
