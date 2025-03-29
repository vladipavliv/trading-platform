/**
 * @author Vladimir Pavliv
 * @date 2025-03-23
 */

#ifndef HFT_COMMON_TCPTRANSPORT_HPP
#define HFT_COMMON_TCPTRANSPORT_HPP

#include "bus/bus.hpp"
#include "logger.hpp"
#include "market_types.hpp"
#include "network/connection_status.hpp"
#include "network/ring_buffer.hpp"
#include "template_types.hpp"
#include "types.hpp"

namespace hft {

/**
 * @brief
 */
template <typename FramerType>
class TcpTransport {
public:
  using Framer = FramerType;
  using UPtr = std::unique_ptr<TcpTransport>;

  TcpTransport(TcpSocket socket, BusWrapper bus)
      : socket_{std::move(socket)}, bus_{std::move(bus)} {}

  TcpTransport(TcpSocket socket, TcpEndpoint endpoint, BusWrapper bus)
      : socket_{std::move(socket)}, endpoint_{std::move(endpoint)}, bus_{std::move(bus)} {}

  void connect() {
    socket_.async_connect(endpoint_, [this](CRef<BoostError> code) {
      if (!code) {
        socket_.set_option(TcpSocket::protocol_type::no_delay(true));
        onConnected();
      } else {
        spdlog::error("TcpTransport error {} {}", bus_.traderId(), code.message());
        onError();
      }
      bus_.post(TcpConnectionStatus{bus_.traderId(), status_});
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
    auto data = std::make_shared<ByteBuffer>(Framer::frame(messages));
    boost::asio::async_write(
        socket_, boost::asio::buffer(data->data(), data->size()),
        [this, data](CRef<BoostError> code, size_t bytes) { writeHandler(code, bytes); });
  }

  inline auto status() const -> ConnectionStatus { return status_; }

  inline void close() { socket_.close(); }

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
    Framer::unframe(buffer_, bus_);
    read();
  }

  void writeHandler(CRef<BoostError> code, size_t bytes) {
    if (code) {
      spdlog::error("Tcp error {} {}", bus_.traderId(), code.message());
      onError();
    }
  }

  void onConnected() {
    status_.store(ConnectionStatus::Connected);
    bus_.post(TcpConnectionStatus{bus_.traderId(), ConnectionStatus::Connected});
  }

  void onDisconnected() {
    status_.store(ConnectionStatus::Disconnected);
    bus_.post(TcpConnectionStatus{bus_.traderId(), ConnectionStatus::Disconnected});
  }

  void onError() {
    status_.store(ConnectionStatus::Error);
    bus_.post(TcpConnectionStatus{bus_.traderId(), ConnectionStatus::Error});
  }

private:
  TcpSocket socket_;
  TcpEndpoint endpoint_;

  BusWrapper bus_;
  RingBuffer buffer_;

  Atomic<ConnectionStatus> status_{ConnectionStatus::Disconnected};
};

} // namespace hft

#endif // HFT_COMMON_TCPTRANSPORT_HPP