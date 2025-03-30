/**
 * @author Vladimir Pavliv
 * @date 2025-03-21
 */

#ifndef HFT_COMMON_BUS_HPP
#define HFT_COMMON_BUS_HPP

#include "io_ctx.hpp"
#include "market_types.hpp"
#include "message_bus.hpp"
#include "system_bus.hpp"
#include "template_types.hpp"

namespace hft {

/**
 * @brief Holds MarketBus and SystemBus
 * @details If message is routed by MarketBus - it will be posted to MarketBus
 * otherwise it will be posted to SystemBus
 */
struct Bus {
  using MarketBus = MessageBus<LoginRequest, LoginResponse, Order, OrderStatus, TickerPrice>;

  SystemBus systemBus;
  MarketBus marketBus;

  inline BoostIoCtx &systemCtx() { return systemBus.ioCtx.ctx; }

  template <typename MessageType>
    requires Bus::MarketBus::RoutedType<MessageType>
  void post(Span<MessageType> messages) {
    marketBus.post(messages);
  }

  template <typename MessageType>
    requires(Bus::MarketBus::RoutedType<MessageType>)
  void post(CRef<MessageType> message) {
    marketBus.post(message);
  }

  template <typename MessageType>
    requires(!Bus::MarketBus::RoutedType<MessageType>)
  void post(CRef<MessageType> message) {
    systemBus.post(message);
  }
};
} // namespace hft

#endif // HFT_COMMON_BUS_HPP
