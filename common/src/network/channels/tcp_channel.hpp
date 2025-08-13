/**
 * @author Vladimir Pavliv
 * @date 2025-03-23
 */

#ifndef HFT_COMMON_TCPCHANNEL_HPP
#define HFT_COMMON_TCPCHANNEL_HPP

#include "domain_types.hpp"
#include "logging.hpp"
#include "network/connection_status.hpp"
#include "network/framing/framer.hpp"
#include "network/ring_buffer.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft {

/**
 * @brief Asynchronous TcpSocket wrapper
 * @details Reads from the socket, unframes with FramerType, and posts to consumer
 */
template <typename ConsumerType, typename FramerType = DefaultFramer>
class TcpChannel {
public:
  using Consumer = ConsumerType;
  using Framer = FramerType;

  TcpChannel(ConnectionId id, TcpSocket socket, Consumer &consumer)
      : id_{id}, socket_{std::move(socket)}, consumer_{consumer},
        status_{ConnectionStatus::Connected} {}

  TcpChannel(ConnectionId id, TcpSocket socket, TcpEndpoint endpoint, Consumer &consumer)
      : TcpChannel(id, std::move(socket), consumer) {
    endpoint_ = std::move(endpoint);
    status_ = ConnectionStatus::Disconnected;
  }

  void connect() {
    LOG_DEBUG("TcpChannel connect");
    socket_.async_connect(endpoint_, [this](BoostErrorCode code) {
      if (!code) {
        socket_.set_option(TcpSocket::protocol_type::no_delay(true));
        onStatus(ConnectionStatus::Connected);
        read();
      } else {
        onStatus(ConnectionStatus::Error);
      }
    });
  }

  void reconnect() {
    LOG_DEBUG("TcpChannel reconnect");
    socket_.close();
    connect();
  }

  void read() {
    LOG_TRACE("TcpChannel read");
    socket_.async_read_some(
        buffer_.buffer(), [this](BoostErrorCode code, size_t bytes) { readHandler(code, bytes); });
  }

  template <typename Type>
    requires(Framer::template Framable<Type>)
  void write(CRef<Type> msg) {
    LOG_TRACE("TcpChannel write {}", utils::toString(msg));
    const auto buffer = std::make_shared<ByteBuffer>();
    Framer::frame(msg, *buffer);
    boost::asio::async_write(
        socket_, boost::asio::buffer(buffer->data(), buffer->size()),
        [this, buffer](BoostErrorCode code, size_t bytes) { writeHandler(code, bytes); });
  }

  inline ConnectionId id() const { return id_; }

  inline auto status() const -> ConnectionStatus { return status_; }

  inline auto isConnected() const -> bool { return status_ == ConnectionStatus::Connected; }

  inline auto isError() const -> bool { return status_ == ConnectionStatus::Error; }

  inline void close() {
    socket_.close();
    status_ = ConnectionStatus::Disconnected;
  }

private:
  void readHandler(BoostErrorCode code, size_t bytes) {
    if (code) {
      buffer_.reset();
      if (code != ASIO_ERR_ABORTED) {
        onStatus(ConnectionStatus::Error);
      }
      return;
    }
    buffer_.commitWrite(bytes);
    const auto res = Framer::unframe(buffer_.data(), consumer_);
    if (res) {
      buffer_.commitRead(*res);
    } else {
      LOG_ERROR("{}", utils::toString(res.error()));
      buffer_.reset();
      onStatus(ConnectionStatus::Error);
      return;
    }
    read();
  }

  void writeHandler(BoostErrorCode code, size_t bytes) {
    if (code && code != ASIO_ERR_ABORTED) {
      onStatus(ConnectionStatus::Error);
    }
  }

  void onStatus(ConnectionStatus status) {
    status_.store(status);
    consumer_.post(ConnectionStatusEvent{id_, status});
  }

private:
  const ConnectionId id_;

  RingBuffer buffer_;
  Consumer &consumer_;

  TcpEndpoint endpoint_;
  TcpSocket socket_;

  Atomic<ConnectionStatus> status_{ConnectionStatus::Disconnected};
};

} // namespace hft

#endif // HFT_COMMON_TCPTRANSPORT_HPP
