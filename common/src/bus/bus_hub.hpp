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
template <typename MarketBusT, typename StreamBusT>
struct BusHub {
public:
  BusHub() : streamBus{systemBus} {}

  SystemBus systemBus;
  MarketBusT marketBus;
  StreamBusT streamBus;

  inline IoCtx &systemIoCtx() { return systemBus.systemIoCtx(); }
  inline IoCtx &streamIoCtx() { return streamBus.streamIoCtx(); }

  void run() {
    streamBus.run();
    systemBus.run();
  }

  void stop() {
    streamBus.stop();
    systemBus.stop();
  }

  template <typename Message>
    requires(MarketBusT::template Routed<Message> && !StreamBusT::template Routed<Message>)
  inline void post(CRef<Message> message) {
    marketBus.template post<Message>(message);
  }

  template <typename Message>
    requires(MarketBusT::template Routed<Message> && !StreamBusT::template Routed<Message>)
  inline void subscribe(CRefHandler<Message> &&handler) {
    marketBus.template subscribe<Message>(std::move(handler));
  }

  template <typename Message>
    requires(!MarketBusT::template Routed<Message> && StreamBusT::template Routed<Message>)
  inline void post(CRef<Message> message) {
    streamBus.template post<Message>(message);
  }

  template <typename Message>
    requires(!MarketBusT::template Routed<Message> && StreamBusT::template Routed<Message>)
  inline void subscribe(CRefHandler<Message> &&handler) {
    streamBus.template subscribe<Message>(std::move(handler));
  }

  template <typename Message>
    requires(!MarketBusT::template Routed<Message> && !StreamBusT::template Routed<Message>)
  inline void post(CRef<Message> message) {
    systemBus.template post<Message>(message);
  }

  template <typename Message>
    requires(!MarketBusT::template Routed<Message> && !StreamBusT::template Routed<Message>)
  inline void subscribe(CRefHandler<Message> &&handler) {
    systemBus.template subscribe<Message>(std::move(handler));
  }

  template <utils::UnorderedMapKey EventType>
  void subscribe(EventType event, Callback &&callback) {
    systemBus.subscribe(event, std::move(callback));
  }

  template <typename Message>
    requires(MarketBusT::template Routed<Message> && StreamBusT::template Routed<Message>)
  inline void post(CRef<Message>) = delete;

  template <typename Message>
    requires(MarketBusT::template Routed<Message> && StreamBusT::template Routed<Message>)
  inline void subscribe(CRefHandler<Message>) = delete;
};
} // namespace hft

#endif // HFT_COMMON_BUSHUB_HPP
