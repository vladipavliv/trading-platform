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
 * @todo Try combine system and market buses and specify special market types via variadics
 */
struct Bus {
  using MarketBus = MessageBus<Order, OrderStatus, TickerPrice>;

  SystemBus systemBus;
  MarketBus marketBus;

  inline IoContext &ioCtx() { return systemBus.systemIoCtx; }

  void run() { systemBus.systemIoCtx.run(); }
  void stop() { systemBus.systemIoCtx.stop(); }
};

class BusWrapper {
  template <typename MessageType, typename = void>
  struct HasTraderId : std::false_type {};

  // Specialization if T has member 'traderId'
  template <typename MessageType>
  struct HasTraderId<MessageType, std::void_t<decltype(std::declval<MessageType>().traderId)>>
      : std::true_type {};

public:
  explicit BusWrapper(Bus &bus, TraderId traderId = 0) : bus_{bus}, traderId_{traderId} {}

  inline TraderId traderId() const { return traderId_; }

  template <typename MessageType>
    requires Bus::MarketBus::RoutedType<MessageType>
  void post(Span<MessageType> messages) {
    if constexpr (HasTraderId<MessageType>::value) {
      for (auto &message : messages) {
        message.traderId = traderId_;
      }
    }
    bus_.marketBus.post(messages);
  }

  template <typename MessageType>
    requires(Bus::MarketBus::RoutedType<MessageType>)
  void post(Ref<MessageType> message) {
    if constexpr (HasTraderId<MessageType>::value) {
      message.traderId = traderId_;
    }
    bus_.marketBus.post(Span<MessageType>(&message, 1));
  }

  template <typename MessageType>
    requires(!Bus::MarketBus::RoutedType<MessageType>)
  void post(CRef<MessageType> message) {
    bus_.systemBus.post(message);
  }

private:
  Bus &bus_;
  TraderId traderId_;
};
} // namespace hft

#endif // HFT_COMMON_BUS_HPP
