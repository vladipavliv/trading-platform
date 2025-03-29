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

  UdpTransport(UdpSocket socket, UdpEndpoint endpoint, BusWrapper bus)
      : socket_{std::move(socket)}, endpoint_{std::move(endpoint)}, bus_{std::move(bus)} {}

  void read() {
    socket_.async_receive_from(
        buffer_.buffer(), endpoint_,
        [this](CRef<BoostError> code, size_t bytes) { readHandler(code, bytes); });
  }

  template <typename Type>
    requires(Bus::MarketBus::RoutedType<Type>)
  void write(Span<Type> messages) {
    auto data = std::make_shared<ByteBuffer>(Framer::frame(messages));
    socket_.async_send_to(
        ConstBuffer{data->data(), data->size()}, endpoint_,
        [this, data](CRef<BoostError> code, size_t bytes) {
          if (code) {
            Logger::monitorLogger->error("Udp transport error {}", code.message());
            bus_.post(UdpConnectionStatus{ConnectionStatus::Error});
          }
          if (bytes != data->size()) {
            Logger::monitorLogger->error("Failed to write {}, written {}", data->size(), bytes);
            bus_.post(UdpConnectionStatus{ConnectionStatus::Error});
          }
        });
  }

  inline void close() { socket_.close(); }

private:
  void readHandler(CRef<BoostError> code, size_t bytes) {
    if (code) {
      buffer_.reset();
      if (code != boost::asio::error::eof) {
        spdlog::error(code.message());
      }
      bus_.post(UdpConnectionStatus{ConnectionStatus::Error});
      return;
    }
    buffer_.commitWrite(bytes);
    Framer::unframe(buffer_, bus_);
    read();
  }

private:
  UdpSocket socket_;
  UdpEndpoint endpoint_;

  BusWrapper bus_;
  RingBuffer buffer_;
};

} // namespace hft

#endif // HFT_COMMON_UDPTRANSPORT_HPP