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
#include "network/routed_types.hpp"
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

  TcpTransport(TcpSocket socket, Bus &bus, TraderId traderId)
      : socket_{std::move(socket)}, bus_{bus}, traderId_{traderId} {}

  TcpTransport(TcpSocket socket, TcpEndpoint endpoint, Bus &bus)
      : socket_{std::move(socket)}, endpoint_{std::move(endpoint)}, bus_{bus} {}

  void connect() {
    socket_.async_connect(endpoint_, [this](CRef<BoostError> code) {
      if (!code) {
        socket_.set_option(TcpSocket::protocol_type::no_delay(true));
        onConnected();
      } else {
        Logger::monitorLogger->error("TcpTransport error {} {}", traderId_, code.message());
        onError();
      }
      bus_.systemBus.post(TcpConnectionStatus{traderId_, status_});
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

  template <RoutedType Type>
  void write(Span<Type> messages) {
    ByteBuffer data = Framer::frame(messages);
    boost::asio::async_write(
        socket_, boost::asio::buffer(data.data(), data.size()),
        [this, data = std::move(data)](CRef<BoostError> code, size_t bytes) {
          if (code) {
            Logger::monitorLogger->error("Tcp transport error {} {}", traderId_, code.message());
          }
          if (bytes != data.size()) {
            Logger::monitorLogger->error("Failed to write {}bytes, written {}", data.size(), bytes);
          }
        });
  }

  inline auto status() const -> ConnectionStatus { return status_; }

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

  void onConnected() {
    status_.store(ConnectionStatus::Connected);
    bus_.systemBus.post(TcpConnectionStatus{traderId_, ConnectionStatus::Connected});
  }

  void onDisconnected() {
    status_.store(ConnectionStatus::Disconnected);
    bus_.systemBus.post(TcpConnectionStatus{traderId_, ConnectionStatus::Disconnected});
  }

  void onError() {
    status_.store(ConnectionStatus::Error);
    bus_.systemBus.post(TcpConnectionStatus{traderId_, ConnectionStatus::Error});
  }

private:
  TcpSocket socket_;
  TcpEndpoint endpoint_;

  Bus &bus_;
  RingBuffer buffer_;
  TraderId traderId_{0};

  Atomic<ConnectionStatus> status_{ConnectionStatus::Disconnected};
};

} // namespace hft

#endif // HFT_COMMON_TCPTRANSPORT_HPP