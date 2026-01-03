/**
 * @author Vladimir Pavliv
 * @date 2025-03-23
 */

#ifndef HFT_COMMON_UDPCHANNEL_HPP
#define HFT_COMMON_UDPCHANNEL_HPP

#include "concepts/busable.hpp"
#include "domain_types.hpp"
#include "logging.hpp"
#include "network/buffer_pool.hpp"
#include "network/connection_status.hpp"
#include "network/framing/framer.hpp"
#include "network/ring_buffer.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

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
    utils::OpCounter guard{&activeOps_};
    socket_.async_receive_from( // format
        buffer_.buffer(), endpoint_,
        [this, guard = std::move(guard)](BoostErrorCode code, size_t bytes) {
          readHandler(code, bytes);
        });
  }

  template <typename Type>
  void write(CRef<Type> message) {
    utils::OpCounter guard{&activeOps_};
    const auto buffer = BufferPool<>::instance().acquire();
    const auto msgSize = Framer::frame(message, buffer.data);

    socket_.async_send_to( // format
        boost::asio::buffer(buffer.data, msgSize), endpoint_,
        [this, buffer, msgSize, guard = std::move(guard)](BoostErrorCode code, size_t bytes) {
          BufferPool<>::instance().release(buffer.index);
          if (code) {
            LOG_ERROR("Udp transport error {}", code.message());
            bus_.post(ConnectionStatusEvent{id_, ConnectionStatus::Error});
          }
          if (bytes != msgSize) {
            LOG_ERROR("Failed to write {}, written {}", msgSize, bytes);
            bus_.post(ConnectionStatusEvent{id_, ConnectionStatus::Error});
          }
        });
  }

  inline ConnectionId id() const { return id_; }

  inline void close() {
    socket_.close();
    while (activeOps_.load(std::memory_order_acquire) > 0) {
      asm volatile("pause" ::: "memory");
    }
  }

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

  std::atomic_size_t activeOps_{0};
};

} // namespace hft

#endif // HFT_COMMON_UDPCHANNEL_HPP
