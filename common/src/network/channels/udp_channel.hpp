/**
 * @author Vladimir Pavliv
 * @date 2025-03-23
 */

#ifndef HFT_COMMON_UDPCHANNEL_HPP
#define HFT_COMMON_UDPCHANNEL_HPP

#include "concepts/busable.hpp"
#include "domain_types.hpp"
#include "logging.hpp"
#include "network/connection_status.hpp"
#include "network/framing/framer.hpp"
#include "network/ring_buffer.hpp"
#include "types.hpp"

namespace hft {

/**
 * @brief Asynchronous UdpSocket wrapper
 * @details Reads messages from the socket, unframes with FramerType, and posts to a bus
 */
template <Busable Bus, typename Framer = DefaultFramer>
class UdpChannel {
public:
  using BusType = Bus;
  using FramerType = Framer;

  UdpChannel(ConnectionId id, UdpSocket socket, UdpEndpoint endpoint, Bus &bus)
      : id_{id}, socket_{std::move(socket)}, endpoint_{std::move(endpoint)}, bus_{bus} {}

  void read() {
    socket_.async_receive_from(
        buffer_.buffer(), endpoint_,
        [this](BoostErrorCode code, size_t bytes) { readHandler(code, bytes); });
  }

  template <typename Type>
  auto write(CRef<Type> message) -> StatusCode {
    const auto buffer = std::make_shared<ByteBuffer>();
    Framer::frame(message, *buffer);
    socket_.async_send_to( // format
        boost::asio::buffer(buffer->data(), buffer->size()), endpoint_,
        [this, buffer](BoostErrorCode code, size_t bytes) {
          if (code) {
            LOG_ERROR("Udp transport error {}", code.message());
            bus_.post(ConnectionStatusEvent{id_, ConnectionStatus::Error});
          }
          if (bytes != buffer->size()) {
            LOG_ERROR("Failed to write {}, written {}", buffer->size(), bytes);
            bus_.post(ConnectionStatusEvent{id_, ConnectionStatus::Error});
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
      bus_.post(ConnectionStatusEvent{id_, ConnectionStatus::Error});
      return;
    }
    buffer_.commitWrite(bytes);
    const auto res = Framer::unframe(buffer_.data(), bus_);
    if (res) {
      buffer_.commitRead(*res);
    } else {
      LOG_ERROR("{}", utils::toString(res.error()));
      buffer_.reset();
      bus_.post(ConnectionStatusEvent{id_, ConnectionStatus::Error});
      return;
    }
    read();
  }

private:
  const ConnectionId id_;

  UdpSocket socket_;
  UdpEndpoint endpoint_;

  Bus &bus_;
  RingBuffer buffer_;
};

} // namespace hft

#endif // HFT_COMMON_UDPCHANNEL_HPP
