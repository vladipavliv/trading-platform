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

/**
 * @brief Sets the SocketId to a posted message and redirects it to a Consumer
 * Market messages contain Token so associating message with the socket is not needed
 * But for login messages it is needed. Such appender simplifies it for the flow
 * Transport -> Framer -> Serializer -> Consumer
 */
template <typename Consumer>
class SocketIdAppender {
public:
  SocketIdAppender(Consumer &consumer, SocketId id) : consumer_{consumer}, id_{id} {}

  template <typename MessageType>
  void post(CRef<MessageType> message) {
    if constexpr (MutableSocketId<MessageType>) {
      message.setSocketId(id_);
    }
    consumer_.post(message);
  }

private:
  Consumer &consumer_;
  const SocketId id_;
};

/**
 * @brief Appends TraderId
 */
template <typename Consumer>
class TraderIdAppender {
public:
  TraderIdAppender(Consumer &consumer, TraderId id) : consumer_{consumer}, id_{id} {}

  template <typename MessageType>
  void post(CRef<MessageType> message) {
    if constexpr (MutableTraderId<MessageType>) {
      message.setTraderId(id_);
    }
    consumer_.post(message);
  }

private:
  Consumer &consumer_;
  const TraderId id_;
};
} // namespace hft

#endif // HFT_COMMON_APPENDER_HPP