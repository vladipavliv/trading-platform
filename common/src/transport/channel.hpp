/**
 * @author Vladimir Pavliv
 * @date 2025-03-23
 */

#ifndef HFT_COMMON_CHANNEL_HPP
#define HFT_COMMON_CHANNEL_HPP

#include "bus/busable.hpp"
#include "containers/buffer_pool.hpp"
#include "containers/sliding_buffer.hpp"
#include "domain_types.hpp"
#include "logging.hpp"
#include "network_traits.hpp"
#include "primitive_types.hpp"
#include "transport/connection_status.hpp"
#include "transport/transport.hpp"
#include "utils/string_utils.hpp"
#include <cassert>

namespace hft {

/**
 * @brief performs framing, serializing, and routing messages network <-> system
 */
template <typename TransportT, typename BusT>
class Channel : public std::enable_shared_from_this<Channel<TransportT, BusT>> {
public:
  static_assert(Transport<TransportT, std::function<void(IoResult)>>,
                "TransportT must satisfy the Transport concept");
  static_assert(Busable<BusT>, "BusT must satisfy the Busable concept");

  Channel(TransportT &&transport, ConnectionId id, BusT &&bus)
      : transport_{std::move(transport)}, id_{id}, bus_{std::move(bus)} {
    LOG_DEBUG("Channel ctor");
  }

  ~Channel() {
    LOG_DEBUG("Channel dtor");
    close();
  }

  void read() {
    LOG_DEBUG("Channel read");
    if (status_ != ConnectionStatus::Connected) {
      return;
    }
    transport_.asyncRx( // format
        buffer_.buffer(), [self = this->weak_from_this()](IoResult res) {
          auto sharedSelf = self.lock();
          if (!sharedSelf) {
            LOG_ERROR("Channel vanished");
            return;
          }
          sharedSelf->readHandler(res);
        });
  }

  template <typename Type>
    requires(Framer::template Framable<Type>)
  void write(CRef<Type> msg) {
    if (status_ != ConnectionStatus::Connected) {
      return;
    }

    BufferPtr netBuff{BufferPool<>::instance().acquire()};
    if (!netBuff) {
      LOG_ERROR_SYSTEM("Failed to acquire network buffer, message dropped");
      return;
    }
    const auto size = Framer::frame(msg, netBuff.data);
    assert(size < BufferPool<>::BUFFER_CAPACITY);

    const auto dataSpan = ByteSpan{netBuff.data, size};
    LOG_TRACE("sending {} bytes", size);
    transport_.asyncTx( // format
        dataSpan, [self = this->weak_from_this(), idx = netBuff.index](IoResult res) {
          BufferPool<>::instance().release(idx);
          auto sharedSelf = self.lock();
          if (!sharedSelf) {
            LOG_ERROR("Channel vanished");
            return;
          }
          sharedSelf->writeHandler(res);
        });
  }

  inline ConnectionId id() const noexcept { return id_; }

  inline BusT &bus() noexcept { return bus_; }

  inline auto status() const noexcept -> ConnectionStatus { return status_; }

  inline auto isConnected() const noexcept -> bool {
    return status_ == ConnectionStatus::Connected;
  }

  inline auto isError() const noexcept -> bool { return status_ == ConnectionStatus::Error; }

  inline void close() {
    LOG_DEBUG("Channel close");
    transport_.close();
    status_ = ConnectionStatus::Disconnected;
  }

private:
  void readHandler(IoResult res) {
    if (res.code != IoStatus::Ok) {
      buffer_.reset();
      LOG_ERROR("readHandler error");
      onStatus(ConnectionStatus::Error);
      return;
    }
    buffer_.commitWrite(res.bytes);
    const auto unfr = Framer::unframe(buffer_.data(), bus_);
    if (unfr) {
      buffer_.commitRead(*unfr);
    } else {
      LOG_ERROR("Failed to unframe message {}", toString(unfr.error()));
      buffer_.reset();
      onStatus(ConnectionStatus::Error);
      return;
    }
    read();
  }

  void writeHandler(IoResult res) {
    if (res.code != IoStatus::Ok && res.code != IoStatus::Closed) {
      onStatus(ConnectionStatus::Error);
    }
  }

  void onStatus(ConnectionStatus status) {
    status_.store(status);
    bus_.post(ConnectionStatusEvent{id_, status});
  }

private:
  const ConnectionId id_;

  BusT bus_;
  TransportT transport_;
  SlidingBuffer buffer_;

  Atomic<ConnectionStatus> status_{ConnectionStatus::Connected};
};

} // namespace hft

#endif // HFT_COMMON_TCPTRANSPORT_HPP
