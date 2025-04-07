/**
 * @author Vladimir Pavliv
 * @date 2025-03-21
 */

#ifndef HFT_COMMON_BUS_HPP
#define HFT_COMMON_BUS_HPP

#include "market_types.hpp"
#include "message_bus.hpp"
#include "system_bus.hpp"
#include "template_types.hpp"

namespace hft {

/**
 * @brief Holds MarketBus and SystemBus
 * @details Routes message to market or system bus depending on
 * whether its routable by market bus or not
 */
struct Bus {
  using MarketBus = MessageBus<Order, OrderStatus, TickerPrice>;

  SystemBus systemBus;
  MarketBus marketBus;

  inline IoCtx &systemCtx() { return systemBus.ioCtx; }

  template <typename MessageType>
    requires Bus::MarketBus::RoutedType<MessageType>
  void post(CRef<MessageType> message) {
    marketBus.post<MessageType>(message);
  }

  template <typename MessageType>
    requires(!Bus::MarketBus::RoutedType<MessageType>)
  void post(CRef<MessageType> message) {
    systemBus.post<MessageType>(message);
  }
};
} // namespace hft

#endif // HFT_COMMON_BUS_HPP
