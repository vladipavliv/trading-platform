/**
 * @author Vladimir Pavliv
 * @date 2025-03-23
 */

#ifndef HFT_COMMON_TCPTRANSPORT_HPP
#define HFT_COMMON_TCPTRANSPORT_HPP

#include "appender.hpp"
#include "bus/bus.hpp"
#include "bus/subscription_holder.hpp"
#include "logging.hpp"
#include "market_types.hpp"
#include "network/ring_buffer.hpp"
#include "network/size_framer.hpp"
#include "network/socket_status.hpp"
#include "template_types.hpp"
#include "types.hpp"

namespace hft {

/**
 * @brief Asynchronous TcpSocket wrapper
 * @details Reads from the socket, unframes with FramerType, and posts to the gateway
 */
template <typename GatewayType, typename FramerType = SizeFramer<>>
class TcpTransport {
public:
  using Gateway = GatewayType;
  using Framer = FramerType;

  TcpTransport(TcpSocket socket, Gateway &gateway)
      : socket_{std::move(socket)}, gateway_{gateway}, idAppender_{gateway_, id_},
        status_{SocketStatus::Connected} {}

  TcpTransport(TcpSocket socket, TcpEndpoint endpoint, Gateway &gateway)
      : TcpTransport(std::move(socket), gateway) {
    endpoint_ = std::move(endpoint);
    status_ = SocketStatus::Disconnected;
  }

  void connect() {
    LOG_DEBUG("TcpTransport connect");
    socket_.async_connect(endpoint_, [this](CRef<BoostError> code) {
      if (!code) {
        socket_.set_option(TcpSocket::protocol_type::no_delay(true));
        LOG_DEBUG("TcpTransport connected");
        onStatus(SocketStatus::Connected);
        read();
      } else {
        LOG_ERROR("{} {}", id(), code.message());
        onStatus(SocketStatus::Error);
      }
    });
  }

  void reconnect() {
    LOG_DEBUG("TcpTransport reconnect");
    socket_.close();
    connect();
  }

  void read() {
    LOG_DEBUG("TcpTransport read");
    socket_.async_read_some(buffer_.buffer(), [this](CRef<BoostError> code, size_t bytes) {
      readHandler(code, bytes);
    });
  }

  template <typename Type>
    requires(Framer::template Framable<Type>)
  void write(CRef<Type> message) {
    LOG_DEBUG("TcpTransport write");
    auto data = Framer::frame(message);
    boost::asio::async_write(
        socket_, boost::asio::buffer(data->data(), data->size()),
        [this, data](CRef<BoostError> code, size_t bytes) { writeHandler(code, bytes); });
  }

  inline auto status() const -> SocketStatus { return status_; }

  inline void close() { socket_.close(); }

  inline SocketId id() const { return id_; }

private:
  void readHandler(CRef<BoostError> code, size_t bytes) {
    if (code) {
      buffer_.reset();
      if (code != boost::asio::error::eof) {
        LOG_ERROR_SYSTEM("TcpTransport error {}", code.message());
      }
      onStatus(SocketStatus::Error);
      return;
    }
    buffer_.commitWrite(bytes);
    Framer::unframe(buffer_, idAppender_);
    read();
  }

  void writeHandler(CRef<BoostError> code, size_t bytes) {
    if (code) {
      LOG_ERROR("TcpTransport error {} {}", id(), code.message());
      onStatus(SocketStatus::Error);
    }
  }

  void onStatus(SocketStatus status) {
    status_.store(status);
    gateway_.post(SocketStatusEvent{id_, status});
  }

private:
  const SocketId id_{utils::generateSocketId()};

  TcpSocket socket_;
  TcpEndpoint endpoint_;

  Gateway &gateway_;
  RingBuffer buffer_;

  SocketIdAppender<Gateway> idAppender_;

  Atomic<SocketStatus> status_{SocketStatus::Disconnected};
};

} // namespace hft

#endif // HFT_COMMON_TCPTRANSPORT_HPP