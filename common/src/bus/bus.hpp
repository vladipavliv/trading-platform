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
 * @details Routes message to market or system bus depending on
 * whether its routable by market bus or not
 */
template <typename... MarketTypes>
struct BusHolder {
  using MarketBus = MessageBus<MarketTypes...>;

  SystemBus systemBus;
  MarketBus marketBus;

  inline IoCtx &systemCtx() { return systemBus.ioCtx; }

  template <typename MessageType>
    requires(MarketBus::template RoutedType<MessageType>)
  void post(CRef<MessageType> message) {
    marketBus.template post<MessageType>(message);
  }

  template <typename MessageType>
    requires(!MarketBus::template RoutedType<MessageType>)
  void post(CRef<MessageType> message) {
    systemBus.template post<MessageType>(message);
  }
};
} // namespace hft

#endif // HFT_COMMON_BUS_HPP
