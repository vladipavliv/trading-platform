/**
 * @author Vladimir Pavliv
 * @date 2025-04-06
 */

#ifndef HFT_COMMON_APPENDER_HPP
#define HFT_COMMON_APPENDER_HPP

#include "bus/bus.hpp"
#include "template_types.hpp"
#include "types.hpp"

namespace hft {

template <typename Consumer>
class SocketIdAppender {
public:
  SocketIdAppender(Consumer &consumer, SocketId id) : consumer_{consumer}, id_{id} {}

  template <typename MessageType>
  void post(CRef<MessageType> message) {
    if constexpr (MutableSocketId<MessageType>) {
      message.socketId = id_;
    }
    consumer_.post(message);
  }

private:
  Consumer &consumer_;
  const SocketId id_;
};

template <typename Consumer>
class TraderIdAppender {
public:
  TraderIdAppender(Consumer &consumer, TraderId id) : consumer_{consumer}, id_{id} {}

  template <typename MessageType>
  void post(CRef<MessageType> message) {
    if constexpr (MutableTraderId<MessageType>) {
      message.traderId = id_;
    }
    consumer_.post(message);
  }

private:
  Consumer &consumer_;
  const TraderId id_;
};
} // namespace hft

#endif // HFT_COMMON_APPENDER_HPP