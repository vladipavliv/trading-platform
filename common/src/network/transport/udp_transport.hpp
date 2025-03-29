/**
 * @author Vladimir Pavliv
 * @date 2025-03-23
 */

#ifndef HFT_COMMON_UDPTRANSPORT_HPP
#define HFT_COMMON_UDPTRANSPORT_HPP

#include "bus/bus.hpp"
#include "logger.hpp"
#include "market_types.hpp"
#include "network/connection_status.hpp"
#include "network/ring_buffer.hpp"
#include "network/routed_types.hpp"
#include "network/size_framer.hpp"
#include "template_types.hpp"
#include "types.hpp"

namespace hft {

/**
 * @brief
 */
template <typename FramerType>
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

  template <RoutedType Type>
  void write(Span<Type> messages) {
    ByteBuffer data = Framer::frame(messages);
    socket_.async_send_to(
        ConstBuffer{data.data(), data.size()}, endpoint_,
        [data = std::move(data)](CRef<BoostError> code, size_t bytes) {
          if (code) {
            Logger::monitorLogger->error("Udp transport error {}", code.message());
          }
          if (bytes != data.size()) {
            Logger::monitorLogger->error("Failed to write {}, written {}", data.size(), bytes);
          }
        });
  }

  inline auto status() const -> ConnectionStatus { return status_; }

private:
  void readHandler(CRef<BoostError> code, size_t bytes) {
    if (code) {
      buffer_.reset();
      if (code != boost::asio::error::eof) {
        spdlog::error(code.message());
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
    bus_.systemBus.post(UdpConnectionStatus{ConnectionStatus::Connected});
  }

  void onDisconnected() {
    status_.store(ConnectionStatus::Disconnected);
    bus_.systemBus.post(UdpConnectionStatus{ConnectionStatus::Disconnected});
  }

  void onError() {
    status_.store(ConnectionStatus::Error);
    bus_.systemBus.post(UdpConnectionStatus{ConnectionStatus::Error});
  }

private:
  UdpSocket socket_;
  UdpEndpoint endpoint_;

  Bus &bus_;
  RingBuffer buffer_;

  Atomic<ConnectionStatus> status_{ConnectionStatus::Disconnected};
};

} // namespace hft

#endif // HFT_COMMON_UDPTRANSPORT_HPP