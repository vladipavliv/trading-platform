/**
 * @author Vladimir Pavliv
 * @date 2025-03-21
 */

#ifndef HFT_COMMON_BUS_HPP
#define HFT_COMMON_BUS_HPP

#include "domain_types.hpp"
#include "message_bus.hpp"
#include "system_bus.hpp"

namespace hft {

/**
 * @brief Holds MarketBus and SystemBus
 * @details MarketBus has the types explicitly specified via variadics, so events of those types
 * are routed to MarketBus, other events are routed to SystemBus
 * @todo As of now all bus subscribers are long living objects, so unsub mechanism is not needed
 * Hot market message routing path should be carefully thought-through, thread safety, container
 * efficiency, the current downstream connection changes, all this should be handled elsewhere
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
