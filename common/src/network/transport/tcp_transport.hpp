/**
 * @author Vladimir Pavliv
 * @date 2025-03-23
 */

#ifndef HFT_COMMON_TCPTRANSPORT_HPP
#define HFT_COMMON_TCPTRANSPORT_HPP

#include "bus/bus.hpp"
#include "bus/subscription_holder.hpp"
#include "logger.hpp"
#include "market_types.hpp"
#include "network/connection_status.hpp"
#include "network/ring_buffer.hpp"
#include "template_types.hpp"
#include "types.hpp"

namespace hft {

/**
 * @brief Asynchronous TcpSocket wrapper
 * @details Reads from the socket, unframes with FramerType, and posts to the gateway
 */
template <typename GatewayType, typename FramerType>
class TcpTransport {
public:
  using Gateway = GatewayType;
  using Framer = FramerType;
  using UPtr = std::unique_ptr<TcpTransport>;

  TcpTransport(TcpSocket socket, Gateway gateway, TcpConnectionType type)
      : socket_{std::move(socket)}, gateway_{std::move(gateway)}, type_{type} {}

  TcpTransport(TcpSocket socket, TcpEndpoint endpoint, Gateway gateway, TcpConnectionType type)
      : TcpTransport(std::move(socket), std::move(gateway), type) {
    endpoint_ = std::move(endpoint);
  }

  void connect() {
    socket_.async_connect(endpoint_, [this](CRef<BoostError> code) {
      if (!code) {
        socket_.set_option(TcpSocket::protocol_type::no_delay(true));
        onConnected();
      } else {
        spdlog::error("TcpTransport error {} {}", id(), code.message());
        onError();
      }
      gateway_.post(TcpStatusEvent{id_, 0, status_});
    });
  }

  void reconnect() {
    socket_.close();
    connect();
  }

  void read() {
    socket_.async_read_some(buffer_.buffer(), [this](CRef<BoostError> code, size_t bytes) {
      readHandler(code, bytes);
    });
  }

  template <typename Type>
    requires(Bus::MarketBus::RoutedType<Type>)
  void write(Span<Type> messages) {
    auto data = Framer::frame(messages);
    boost::asio::async_write(
        socket_, boost::asio::buffer(data->data(), data->size()),
        [this, data](CRef<BoostError> code, size_t bytes) { writeHandler(code, bytes); });
  }

  template <typename Type>
    requires(Bus::MarketBus::RoutedType<Type>)
  void write(CRef<Type> message) {
    auto data = Framer::frame(message);
    boost::asio::async_write(
        socket_, boost::asio::buffer(data->data(), data->size()),
        [this, data](CRef<BoostError> code, size_t bytes) { writeHandler(code, bytes); });
  }

  inline auto status() const -> ConnectionStatus { return status_; }

  inline void close() { socket_.close(); }

  inline ObjectId id() const { return id_; }

  inline TcpConnectionType type() const { return type_; }

private:
  void readHandler(CRef<BoostError> code, size_t bytes) {
    if (code) {
      buffer_.reset();
      if (code != boost::asio::error::eof) {
        Logger::monitorLogger->error(code.message());
      }
      onError();
      return;
    }
    buffer_.commitWrite(bytes);
    Framer::unframe(buffer_, gateway_);
    read();
  }

  void writeHandler(CRef<BoostError> code, size_t bytes) {
    if (code) {
      spdlog::error("TcpTransport error {} {}", id(), code.message());
      onError();
    }
  }

  void onConnected() {
    status_.store(ConnectionStatus::Connected);
    gateway_.post(TcpStatusEvent{id_, 0, ConnectionStatus::Connected});
  }

  void onDisconnected() {
    status_.store(ConnectionStatus::Disconnected);
    gateway_.post(TcpStatusEvent{id_, 0, ConnectionStatus::Disconnected});
  }

  void onError() {
    status_.store(ConnectionStatus::Error);
    gateway_.post(TcpStatusEvent{id_, 0, ConnectionStatus::Error});
  }

private:
  const ObjectId id_{utils::getId()};

  TcpSocket socket_;
  TcpEndpoint endpoint_;

  Gateway gateway_;
  RingBuffer buffer_;

  const TcpConnectionType type_;
  Atomic<ConnectionStatus> status_{ConnectionStatus::Disconnected};
};

} // namespace hft

#endif // HFT_COMMON_TCPTRANSPORT_HPP