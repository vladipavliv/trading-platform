/**
 * @author Vladimir Pavliv
 * @date 2025-03-23
 */

#ifndef HFT_COMMON_UDPTRANSPORT_HPP
#define HFT_COMMON_UDPTRANSPORT_HPP

#include "bus/bus.hpp"
#include "logging.hpp"
#include "market_types.hpp"
#include "network/ring_buffer.hpp"
#include "network/size_framer.hpp"
#include "network/socket_status.hpp"
#include "template_types.hpp"
#include "types.hpp"

namespace hft {

/**
 * @brief Asynchronous UdpSocket wrapper
 * @details Reads messages from the socket, unframes with FramerType, and posts to a bus
 */
template <typename FramerType = SizeFramer<>>
class UdpTransport {
public:
  using Framer = FramerType;
  using UPtr = std::unique_ptr<UdpTransport>;

  UdpTransport(UdpSocket socket, UdpEndpoint endpoint, Bus &bus)
      : socket_{std::move(socket)}, endpoint_{std::move(endpoint)}, bus_{bus} {}

  void read() {
    socket_.async_receive_from(
        buffer_.buffer(), endpoint_,
        [this](CRef<BoostError> code, size_t bytes) { readHandler(code, bytes); });
  }

  template <typename Type>
    requires(Bus::MarketBus::RoutedType<Type>)
  void write(CRef<Type> messages) {
    auto data = Framer::frame(messages);
    socket_.async_send_to(ConstBuffer{data->data(), data->size()}, endpoint_,
                          [this, data](CRef<BoostError> code, size_t bytes) {
                            if (code) {
                              LOG_ERROR("Udp transport error {}", code.message());
                              bus_.post(SocketStatusEvent{id_, SocketStatus::Error});
                            }
                            if (bytes != data->size()) {
                              LOG_ERROR("Failed to write {}, written {}", data->size(), bytes);
                              bus_.post(SocketStatusEvent{id_, SocketStatus::Error});
                            }
                          });
  }

  inline void close() { socket_.close(); }

  inline SocketId id() const { return id_; }

private:
  void readHandler(CRef<BoostError> code, size_t bytes) {
    if (code) {
      buffer_.reset();
      if (code != boost::asio::error::eof) {
        LOG_ERROR("{}", code.message());
      }
      bus_.post(SocketStatusEvent{id_, SocketStatus::Error});
      return;
    }
    buffer_.commitWrite(bytes);
    Framer::unframe(buffer_, bus_);
    read();
  }

private:
  const ObjectId id_{utils::generateSocketId()};

  UdpSocket socket_;
  UdpEndpoint endpoint_;

  Bus &bus_;
  RingBuffer buffer_;
};

} // namespace hft

#endif // HFT_COMMON_UDPTRANSPORT_HPP