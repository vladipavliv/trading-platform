/**
 * @author Vladimir Pavliv
 * @date 2025-03-23
 */

#ifndef HFT_COMMON_TCPCHANNEL_HPP
#define HFT_COMMON_TCPCHANNEL_HPP

#include "concepts/busable.hpp"
#include "domain_types.hpp"
#include "logging.hpp"
#include "network/buffer_pool.hpp"
#include "network/connection_status.hpp"
#include "network/framing/framer.hpp"
#include "network/ring_buffer.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief Asynchronous TcpSocket wrapper
 * @details Reads from the socket, unframes with FramerType, and posts to the bus
 */
template <Busable Bus, typename Framer = DefaultFramer>
class TcpChannel {
public:
  using BusType = Bus;
  using FramerType = Framer;

  TcpChannel(ConnectionId id, TcpSocket socket, Bus &bus)
      : id_{id}, socket_{std::move(socket)}, bus_{bus}, status_{ConnectionStatus::Connected} {
    setBusyPoll();
  }

  TcpChannel(ConnectionId id, TcpSocket socket, TcpEndpoint endpoint, Bus &bus)
      : TcpChannel(id, std::move(socket), bus) {
    endpoint_ = std::move(endpoint);
    status_ = ConnectionStatus::Disconnected;
  }

  ~TcpChannel() { close(); }

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
    LOG_DEBUG("write {}", activeOps_.load());
    if (status_ != ConnectionStatus::Connected) {
      LOG_ERROR_SYSTEM("read called on disconnected socket");
      return;
    }
    LOG_TRACE("TcpChannel read");
    utils::OpCounter guard{&activeOps_};
    socket_.async_read_some( // format
        buffer_.buffer(), [this, guard = std::move(guard)](BoostErrorCode code, size_t bytes) {
          readHandler(code, bytes);
        });
  }

  template <typename Type>
    requires(Framer::template Framable<Type>)
  void write(CRef<Type> msg) {
    LOG_DEBUG("write {}", activeOps_.load());
    if (status_ != ConnectionStatus::Connected) {
      LOG_ERROR_SYSTEM("write called on disconnected socket");
      return;
    }
    utils::OpCounter guard{&activeOps_};

    LOG_TRACE("TcpChannel write {}", utils::toString(msg));
    const auto buffer = BufferPool<>::instance().acquire();
    const auto msgSize = Framer::frame(msg, buffer.data);
    boost::asio::async_write( // format
        socket_, boost::asio::buffer(buffer.data, msgSize),
        [this, buffer, guard = std::move(guard)](BoostErrorCode code, size_t bytes) {
          BufferPool<>::instance().release(buffer.index);
          writeHandler(code, bytes);
        });
  }

  inline ConnectionId id() const { return id_; }

  inline auto status() const -> ConnectionStatus { return status_; }

  inline auto isConnected() const -> bool { return status_ == ConnectionStatus::Connected; }

  inline auto isError() const -> bool { return status_ == ConnectionStatus::Error; }

  inline void close() {
    LOG_DEBUG("write {}", activeOps_.load());
    boost::system::error_code ec;
    socket_.cancel(ec);
    socket_.close(ec);
    status_ = ConnectionStatus::Disconnected;

    auto ops = activeOps_.load();
    size_t cycles = 0;
    while (activeOps_.load(std::memory_order_acquire) > 0) {
      asm volatile("pause" ::: "memory");
      if (++cycles > 10000000) {
        throw std::runtime_error(
            std::format("Failed to complete socket operations {} {}", ops, activeOps_.load()));
      }
    }
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
    const auto res = Framer::unframe(buffer_.data(), bus_);
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
    bus_.post(ConnectionStatusEvent{id_, status});
  }

  void setBusyPoll(int microseconds = 50) {
    auto native_fd = socket_.native_handle();
    if (setsockopt(native_fd, SOL_SOCKET, SO_BUSY_POLL, &microseconds, sizeof(microseconds)) < 0) {
      LOG_ERROR("Failed to set SO_BUSY_POLL: {}", strerror(errno));
    } else {
      LOG_INFO("SO_BUSY_POLL set to {}us", microseconds);
    }
  }

private:
  const ConnectionId id_;

  RingBuffer buffer_;
  Bus &bus_;

  TcpEndpoint endpoint_;
  TcpSocket socket_;

  std::atomic<size_t> activeOps_{0};
  Atomic<ConnectionStatus> status_{ConnectionStatus::Disconnected};
};

} // namespace hft

#endif // HFT_COMMON_TCPTRANSPORT_HPP
