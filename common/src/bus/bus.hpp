/**
 * @author Vladimir Pavliv
 * @date 2025-03-21
 */

#ifndef HFT_COMMON_BUS_HPP
#define HFT_COMMON_BUS_HPP

#include <concepts>

#include "domain_types.hpp"
#include "message_bus.hpp"
#include "system_bus.hpp"

namespace hft {

/**
 * @brief Holds different types of buses, Routes messages to a proper one
 * @todo As of now all bus subscribers are long living objects, so unsub mechanism is not needed
 */
template <typename... MarketTypes>
struct BusHolder {
  using MarketBus = MessageBus<MarketTypes...>;

  SystemBus systemBus;
  MarketBus marketBus;

  inline IoCtx &systemCtx() { return systemBus.ioCtx; }

  template <typename MessageType>
    requires(MarketBus::template RoutedType<MessageType>)
  inline void post(CRef<MessageType> message) {
    marketBus.template post<MessageType>(message);
  }

  template <typename MessageType>
    requires(!MarketBus::template RoutedType<MessageType>)
  inline void post(CRef<MessageType> message) {
    systemBus.template post<MessageType>(message);
  }

  void run() { systemBus.run(); }

  void stop() { systemBus.stop(); }
};
} // namespace hft

#endif // HFT_COMMON_BUS_HPP
