/**
 * @author Vladimir Pavliv
 * @date 2025-03-21
 */

#ifndef HFT_COMMON_BUSHOLDER_HPP
#define HFT_COMMON_BUSHOLDER_HPP

#include <concepts>

#include "data_bus.hpp"
#include "domain_types.hpp"
#include "message_bus.hpp"
#include "system_bus.hpp"

namespace hft {

/**
 * @brief Holds different types of buses, routes calls to a proper bus
 */
template <typename MarketBus = MessageBus<>, typename StreamBus = DataBus<>>
struct BusHolder {
public:
  SystemBus systemBus;
  MarketBus marketBus;
  StreamBus streamBus;

  inline IoCtx &systemIoCtx() { return systemBus.systemIoCtx(); }
  inline IoCtx &dataIoCtx() { return streamBus.dataIoCtx(); }

  void run() {
    systemBus.run();
    marketBus.run();
    streamBus.run();
  }

  void stop() {
    systemBus.stop();
    marketBus.stop();
    streamBus.stop();
  }

  template <typename Message>
    requires(MarketBus::template Routed<Message> && !StreamBus::template Routed<Message>)
  inline void post(CRef<Message> message) {
    marketBus.template post<Message>(message);
  }

  template <typename Message>
    requires(MarketBus::template Routed<Message> && !StreamBus::template Routed<Message>)
  inline void subscribe(CRefHandler<Message> handler) {
    marketBus.template subscribe<Message>(handler);
  }

  template <typename Message>
    requires(!MarketBus::template Routed<Message> && StreamBus::template Routed<Message>)
  inline void post(CRef<Message> message) {
    streamBus.template post<Message>(message);
  }

  template <typename Message>
    requires(!MarketBus::template Routed<Message> && StreamBus::template Routed<Message>)
  inline void subscribe(CRefHandler<Message> handler) {
    streamBus.template subscribe<Message>(handler);
  }

  template <typename Message>
    requires(!MarketBus::template Routed<Message> && !StreamBus::template Routed<Message>)
  inline void post(CRef<Message> message) {
    systemBus.template post<Message>(message);
  }

  template <typename Message>
    requires(!MarketBus::template Routed<Message> && !StreamBus::template Routed<Message>)
  inline void subscribe(CRefHandler<Message> handler) {
    systemBus.template subscribe<Message>(handler);
  }

  template <UnorderedMapKey EventType>
  void subscribe(EventType event, Callback callback) {
    systemBus.subscribe(event, std::move(callback));
  }

  template <typename Message>
    requires(MarketBus::template Routed<Message> && StreamBus::template Routed<Message>)
  inline void post(CRef<Message>) = delete;

  template <typename Message>
    requires(MarketBus::template Routed<Message> && StreamBus::template Routed<Message>)
  inline void subscribe(CRef<Message>) = delete;
};
} // namespace hft

#endif // HFT_COMMON_BUSHOLDER_HPP
