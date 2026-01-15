/**
 * @author Vladimir Pavliv
 * @date 2025-03-21
 */

#ifndef HFT_COMMON_BUSHUB_HPP
#define HFT_COMMON_BUSHUB_HPP

#include <concepts>

#include "busable.hpp"
#include "domain_types.hpp"
#include "execution.hpp"
#include "message_bus.hpp"
#include "primitive_types.hpp"
#include "stream_bus.hpp"
#include "system_bus.hpp"
#include "utils/trait_utils.hpp"

namespace hft {

/**
 * @brief
 */
template <typename MarketBus>
struct BusHub {
public:
  SystemBus systemBus;
  MarketBus marketBus;

  inline IoCtx &systemIoCtx() { return systemBus.systemIoCtx(); }

  void run() {
#if !defined(BENCHMARK_BUILD) && !defined(UNIT_TESTS_BUILD)
    marketBus.validate();
#endif
    systemBus.run();
  }

  void stop() { systemBus.stop(); }

  template <typename Message>
    requires(MarketBus::template Routed<Message>)
  inline void post(CRef<Message> message) {
    marketBus.template post<Message>(message);
  }

  template <typename Message>
    requires(MarketBus::template Routed<Message>)
  inline void subscribe(CRefHandler<Message> &&handler) {
    marketBus.template subscribe<Message>(std::move(handler));
  }

  template <typename Message>
    requires(!MarketBus::template Routed<Message>)
  inline void post(CRef<Message> message) {
    systemBus.template post<Message>(message);
  }

  template <typename Message>
    requires(!MarketBus::template Routed<Message>)
  inline void subscribe(CRefHandler<Message> &&handler) {
    systemBus.template subscribe<Message>(std::move(handler));
  }

  template <utils::UnorderedMapKey EventType>
  void subscribe(EventType event, Callback &&callback) {
    systemBus.subscribe(event, std::move(callback));
  }
};
} // namespace hft

#endif // HFT_COMMON_BUSHUB_HPP
