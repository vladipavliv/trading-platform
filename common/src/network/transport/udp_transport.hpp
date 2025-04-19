/**
 * @author Vladimir Pavliv
 * @date 2025-03-23
 */

#ifndef HFT_COMMON_UDPTRANSPORT_HPP
#define HFT_COMMON_UDPTRANSPORT_HPP

#include "domain_types.hpp"
#include "logging.hpp"
#include "network/connection_status.hpp"
#include "network/ring_buffer.hpp"
#include "network/size_framer.hpp"
#include "types.hpp"

namespace hft {

/**
 * @brief Asynchronous UdpSocket wrapper
 * @details Reads messages from the socket, unframes with FramerType, and posts to a bus
 */
template <typename ConsumerType, typename FramerType = SizeFramer<>>
class UdpTransport {
public:
  using Consumer = ConsumerType;
  using Framer = FramerType;

  UdpTransport(ConnectionId id, UdpSocket socket, UdpEndpoint endpoint, Consumer &consumer)
      : id_{id}, socket_{std::move(socket)}, endpoint_{std::move(endpoint)}, consumer_{consumer} {}

  void read() {
    socket_.async_receive_from(
        buffer_.buffer(), endpoint_,
        [this](CRef<BoostError> code, size_t bytes) { readHandler(code, bytes); });
  }

  template <typename Type>
  void write(CRef<Type> messages) {
    const auto data = Framer::frame(messages);
    socket_.async_send_to(ConstBuffer{data->data(), data->size()}, endpoint_,
                          [this, data](CRef<BoostError> code, size_t bytes) {
                            if (code) {
                              LOG_ERROR("Udp transport error {}", code.message());
                              consumer_.post(ConnectionStatusEvent{id_, ConnectionStatus::Error});
                            }
                            if (bytes != data->size()) {
                              LOG_ERROR("Failed to write {}, written {}", data->size(), bytes);
                              consumer_.post(ConnectionStatusEvent{id_, ConnectionStatus::Error});
                            }
                          });
  }

  inline ConnectionId id() const { return id_; }

  inline void close() { socket_.close(); }

private:
  void readHandler(CRef<BoostError> code, size_t bytes) {
    if (code) {
      buffer_.reset();
      if (code != boost::asio::error::eof) {
        LOG_ERROR("{}", code.message());
      }
      consumer_.post(ConnectionStatusEvent{id_, ConnectionStatus::Error});
      return;
    }
    buffer_.commitWrite(bytes);
    Framer::unframe(buffer_, consumer_);
    read();
  }

private:
  const ConnectionId id_;

  UdpSocket socket_;
  UdpEndpoint endpoint_;

  Consumer &consumer_;
  RingBuffer buffer_;
};

} // namespace hft

#endif // HFT_COMMON_UDPTRANSPORT_HPP