/**
 * @author Vladimir Pavliv
 * @date 2025-03-23
 */

#ifndef HFT_COMMON_CHANNEL_HPP
#define HFT_COMMON_CHANNEL_HPP

#include "bus/busable.hpp"
#include "domain_types.hpp"
#include "logging.hpp"
#include "network/async_transport.hpp"
#include "network/buffer_pool.hpp"
#include "network/connection_status.hpp"
#include "network/framing/framer.hpp"
#include "network/ring_buffer.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief
 */
template <AsyncTransport T, Busable B, typename F = DefaultFramer>
class Channel {
public:
  using Transport = T;
  using Bus = B;
  using Framer = F;

  Channel(Transport &&transport, ConnectionId id, Bus &bus)
      : transport_{std::move(transport)}, id_{id}, bus_{bus} {
    read();
  }

  ~Channel() { close(); }

  void read() {
    using namespace utils;
    LOG_TRACE("read {}", activeOps_.load());
    if (status_ != ConnectionStatus::Connected) {
      LOG_ERROR_SYSTEM("read called on disconnected socket");
      return;
    }
    transport_.asyncRx(buffer_.buffer(),
                       [this, guard = OpCounter{&activeOps_}](IoResult code, size_t bytes) {
                         readHandler(code, bytes);
                       });
  }

  template <typename Type>
    requires(Framer::template Framable<Type>)
  void write(CRef<Type> msg) {
    using namespace utils;
    LOG_TRACE("write {} active ops: {}", toString(msg), activeOps_.load());
    if (status_ != ConnectionStatus::Connected) {
      LOG_ERROR_SYSTEM("write called on disconnected socket");
      return;
    }

    NetworkBuffer netBuff{BufferPool<>::instance().acquire(), &activeOps_};
    if (!netBuff) {
      LOG_ERROR_SYSTEM("Failed to acquire network buffer, message dropped");
      return;
    }
    netBuff.setSize(Framer::frame(msg, netBuff.data()));
    transport_.asyncTx( // format
        netBuff.dataSpan(), [this, buff = std::move(netBuff)](IoResult code, size_t bytes) {
          BufferPool<>::instance().release(buff.index());
          writeHandler(code, bytes);
        });
  }

  inline ConnectionId id() const { return id_; }

  inline auto status() const -> ConnectionStatus { return status_; }

  inline auto isConnected() const -> bool { return status_ == ConnectionStatus::Connected; }

  inline auto isError() const -> bool { return status_ == ConnectionStatus::Error; }

  inline void close() noexcept {
    try {
      LOG_DEBUG("close {}", activeOps_.load());
      transport_.close();
      status_ = ConnectionStatus::Disconnected;

      size_t cycles = 0;
      while (activeOps_.load(std::memory_order_acquire) > 0) {
        asm volatile("pause" ::: "memory");
        if (++cycles > BUSY_WAIT_CYCLES) {
          throw std::runtime_error(
              std::format("Failed to complete {} socket operations", activeOps_.load()));
        }
      }
    } catch (const std::exception &ex) {
      LOG_ERROR_SYSTEM("exception in Channel::close {}", ex.what());
    } catch (...) {
      LOG_ERROR_SYSTEM("unknown exception in Channel::close");
    }
  }

private:
  void readHandler(IoResult code, size_t bytes) {
    if (code != IoResult::Ok) {
      buffer_.reset();
      LOG_ERROR("readHandler error");
      onStatus(ConnectionStatus::Error);
      return;
    }
    buffer_.commitWrite(bytes);
    const auto res = Framer::unframe(buffer_.data(), bus_);
    if (res) {
      buffer_.commitRead(*res);
    } else {
      LOG_ERROR("Failed to unframe message {}", utils::toString(res.error()));
      buffer_.reset();
      onStatus(ConnectionStatus::Error);
      return;
    }
    read();
  }

  void writeHandler(IoResult code, size_t bytes) {
    if (code != IoResult::Ok) {
      LOG_ERROR("writeHandler error");
      onStatus(ConnectionStatus::Error);
    }
  }

  void onStatus(ConnectionStatus status) {
    status_.store(status);
    bus_.post(ConnectionStatusEvent{id_, status});
  }

private:
  const ConnectionId id_;

  RingBuffer buffer_;
  Bus &bus_;

  Transport transport_;

  Atomic<size_t> activeOps_{0};
  Atomic<ConnectionStatus> status_{ConnectionStatus::Connected};
};

} // namespace hft

#endif // HFT_COMMON_TCPTRANSPORT_HPP
