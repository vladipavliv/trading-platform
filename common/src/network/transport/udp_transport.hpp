/**
 * @author Vladimir Pavliv
 * @date 2025-03-23
 */

#ifndef HFT_COMMON_UDPTRANSPORT_HPP
#define HFT_COMMON_UDPTRANSPORT_HPP

#include "domain_types.hpp"
#include "logging.hpp"
#include "network/connection_status.hpp"
#include "network/framing/fixed_size_framer.hpp"
#include "network/ring_buffer.hpp"
#include "types.hpp"

namespace hft {

/**
 * @brief Asynchronous UdpSocket wrapper
 * @details Reads messages from the socket, unframes with FramerType, and posts to a bus
 */
template <typename ConsumerType, typename FramerType = FixedSizeFramer<>>
class UdpTransport {
public:
  using Consumer = ConsumerType;
  using Framer = FramerType;

  UdpTransport(ConnectionId id, UdpSocket socket, UdpEndpoint endpoint, Consumer &consumer)
      : id_{id}, socket_{std::move(socket)}, endpoint_{std::move(endpoint)}, consumer_{consumer} {}

  void read() {
    socket_.async_receive_from(
        buffer_.buffer(), endpoint_,
        [this](BoostErrorCode code, size_t bytes) { readHandler(code, bytes); });
  }

  template <typename Type>
  StatusCode write(CRef<Type> message) {
    const auto buffer = std::make_shared<ByteBuffer>();
    const auto framingRes = Framer::frame(message, *buffer);
    if (!framingRes) {
      LOG_ERROR("{}", utils::toString(framingRes.error()));
      return framingRes.error();
    }
    LOG_DEBUG("Send {} bytes", *framingRes);
    socket_.async_send_to(boost::asio::buffer(buffer->data(), *framingRes), endpoint_,
                          [this, buffer](BoostErrorCode code, size_t bytes) {
                            if (code) {
                              LOG_ERROR("Udp transport error {}", code.message());
                              consumer_.post(ConnectionStatusEvent{id_, ConnectionStatus::Error});
                            }
                            if (bytes != buffer->size()) {
                              LOG_ERROR("Failed to write {}, written {}", buffer->size(), bytes);
                              consumer_.post(ConnectionStatusEvent{id_, ConnectionStatus::Error});
                            }
                          });
    return StatusCode::Ok;
  }

  inline ConnectionId id() const { return id_; }

  inline void close() { socket_.close(); }

private:
  void readHandler(BoostErrorCode code, size_t bytes) {
    if (code) {
      buffer_.reset();
      if (code != boost::asio::error::eof) {
        LOG_ERROR("{}", code.message());
      }
      consumer_.post(ConnectionStatusEvent{id_, ConnectionStatus::Error});
      return;
    }
    buffer_.commitWrite(bytes);
    const auto res = Framer::unframe(buffer_.data(), consumer_);
    if (res) {
      buffer_.commitRead(*res);
    } else {
      LOG_ERROR("{}", utils::toString(res.error()));
      buffer_.reset();
      consumer_.post(ConnectionStatusEvent{id_, ConnectionStatus::Error});
      return;
    }
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