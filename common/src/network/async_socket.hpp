/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_ASYNCSOCKET_HPP
#define HFT_COMMON_ASYNCSOCKET_HPP

#include <boost/endian/arithmetic.hpp>
#include <boost/endian/conversion.hpp>
#include <memory>
#include <spdlog/spdlog.h>
#include <vector>

#include "boost_types.hpp"
#include "constants.hpp"
#include "network_types.hpp"
#include "serialization/flat_buffers/fb_serializer.hpp"
#include "socket_status.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft {

/**
 * @brief Socket wrapper that operates asynchronously over a given socket type
 * Publishes received messages over the market bus, publishes status notifications about
 * connect/disconnect over a system bus
 * @details MessageTypeIn is specified for deserializing incoming messages,
 * while any type of the message could be sent if serializer supports it
 */
template <typename SocketType, typename EventBusType, typename MessageTypeIn>
class AsyncSocket {
public:
  using Type = AsyncSocket<SocketType, EventBusType, MessageTypeIn>;
  using Bus = EventBusType;
  using Socket = SocketType;
  using Endpoint = Socket::endpoint_type;
  using MessageIn = MessageTypeIn;
  using UPtr = std::unique_ptr<Type>;
  using Serializer = serialization::FlatBuffersSerializer;

  using MsgHandler = CRefHandler<MessageIn>;
  using StatusHandler = CRefHandler<SocketStatus>;

  AsyncSocket(Socket &&socket, Bus &bus, TraderId traderId = 0)
      : socket_{std::move(socket)}, bus_{bus}, traderId_{traderId}, readBuffer_(BUFFER_SIZE) {
    status_.store(SocketStatus::Connected, std::memory_order_release);
  }

  AsyncSocket(Socket &&socket, Bus &bus, Endpoint endpoint) : AsyncSocket(std::move(socket), bus) {
    endpoint_ = std::move(endpoint);
    if constexpr (std::is_same_v<Socket, TcpSocket>) {
      status_.store(SocketStatus::Disconnected, std::memory_order_release);
    }
  }

  void asyncConnect() {
    if constexpr (std::is_same_v<Socket, TcpSocket>) {
      socket_.async_connect(endpoint_, [this](BoostErrorRef ec) {
        if (ec) {
          spdlog::error("Failed to connect socket:{}", ec.message());
          onDisconnected();
          return;
        }
        socket_.set_option(TcpSocket::protocol_type::no_delay(true));
        onConnected();
      });
    } else if constexpr (std::is_same_v<Socket, UdpSocket>) {
      asyncRead();
    }
  }

  void reconnect() {
    socket_.close();
    asyncConnect();
  }

  void asyncRead() {
    size_t writable = readBuffer_.size() - tail_;
    uint8_t *writePtr = readBuffer_.data() + tail_;

    if constexpr (std::is_same_v<Socket, TcpSocket>) {
      socket_.async_read_some(
          boost::asio::buffer(writePtr, writable),
          [this](BoostErrorRef code, size_t bytesRead) { readHandler(code, bytesRead); });
    } else if constexpr (std::is_same_v<Socket, UdpSocket>) {
      socket_.async_receive_from(
          boost::asio::buffer(writePtr, writable), endpoint_,
          [this](BoostErrorRef code, size_t bytesRead) { readHandler(code, bytesRead); });
    }
  }

  template <typename MessageTypeOut>
  void asyncWrite(Span<MessageTypeOut> msgVec) {
    size_t allocSize = msgVec.size() * MAX_SERIALIZED_MESSAGE_SIZE;
    auto dataPtr = std::make_unique<ByteBuffer>(allocSize);

    size_t totalSize{0};
    uint8_t *cursor = dataPtr->data();
    for (auto &msg : msgVec) {
      auto msgSize = serializeMessage(msg, cursor);
      cursor += msgSize;
      totalSize += msgSize;
    }

    if constexpr (std::is_same_v<Socket, TcpSocket>) {
      boost::asio::async_write(socket_, boost::asio::buffer(dataPtr->data(), totalSize),
                               [this, data = std::move(dataPtr)](BoostErrorRef ec, size_t size) {
                                 if (ec) {
                                   spdlog::error("Write failed: {}", ec.message());
                                   onDisconnected();
                                 }
                               });
    } else if constexpr (std::is_same_v<Socket, UdpSocket>) {
      socket_.async_send_to(boost::asio::buffer(dataPtr->data(), totalSize), endpoint_,
                            [this, data = std::move(dataPtr)](BoostErrorRef ec, size_t size) {
                              if (ec) {
                                spdlog::error("Write failed: {}", ec.message());
                                onDisconnected();
                              }
                            });
    }
  }

  template <typename MessageTypeOut>
  void asyncWrite(CRef<MessageTypeOut> msg) {
    auto dataPtr = std::make_unique<ByteBuffer>(MAX_SERIALIZED_MESSAGE_SIZE);

    uint8_t *cursor = dataPtr->data();
    auto msgSize = serializeMessage(msg, cursor);

    if constexpr (std::is_same_v<Socket, TcpSocket>) {
      boost::asio::async_write(socket_, boost::asio::buffer(dataPtr->data(), msgSize),
                               [this, data = std::move(dataPtr)](BoostErrorRef ec, size_t size) {
                                 if (ec) {
                                   spdlog::error("Write failed: {}", ec.message());
                                   onDisconnected();
                                 }
                               });
    } else if constexpr (std::is_same_v<Socket, UdpSocket>) {
      socket_.async_send_to(boost::asio::buffer(dataPtr->data(), msgSize), endpoint_,
                            [this, data = std::move(dataPtr)](BoostErrorRef ec, size_t size) {
                              if (ec) {
                                spdlog::error("Write failed: {}", ec.message());
                                onDisconnected();
                              }
                            });
    }
  }

  inline SocketStatus status() const { return status_.load(std::memory_order_acquire); }

private:
  void onDisconnected() {
    status_.store(SocketStatus::Disconnected);
    bus_.systemBus.publish(SocketStatusEvent{traderId_, SocketStatus::Disconnected});
  }

  void onConnected() {
    status_.store(SocketStatus::Connected);
    bus_.systemBus.publish(SocketStatusEvent{traderId_, SocketStatus::Connected});
  }

  template <typename Type>
  boost::endian::little_int16_at serializeMessage(Type &msg, uint8_t *cursor) {
    auto buffer = Serializer::serialize(msg);
    boost::endian::little_int16_at bodySize = static_cast<MessageSize>(buffer.size());

    std::memcpy(cursor, &bodySize, sizeof(bodySize));
    std::memcpy(cursor + sizeof(bodySize), buffer.data(), buffer.size());

    return bodySize + sizeof(bodySize);
  }

  void readHandler(BoostErrorRef ec, size_t bytesRead) {
    if (ec) {
      head_ = tail_ = 0;
      if (ec != boost::asio::error::eof) {
        spdlog::error(ec.message());
      }
      onDisconnected();
      return;
    }
    std::vector<MessageIn> msgBuffer;
    msgBuffer.reserve(10); // TODO(self) improve
    tail_ += bytesRead;
    while (head_ + sizeof(MessageSize) < tail_) {
      uint8_t *cursor = readBuffer_.data() + head_;
      auto littleBodySize = *reinterpret_cast<const boost::endian::little_int16_at *>(cursor);
      MessageSize bodySize = littleBodySize.value();

      size_t msgEnd = head_ + sizeof(MessageSize) + bodySize;
      if (msgEnd > std::min(BUFFER_SIZE, tail_)) {
        break;
      }
      cursor += sizeof(MessageSize);
      auto result = Serializer::template deserialize<MessageIn>(cursor, bodySize);
      if (!result.ok()) {
        head_ = tail_ = 0;
        break;
      }
      if constexpr (!std::is_same_v<MessageTypeIn, TickerPrice>) {
        result.value.traderId = traderId_;
      }
      msgBuffer.emplace_back(result.value);
      head_ += bodySize + sizeof(MessageSize);
    }
    if (!msgBuffer.empty()) {
      bus_.marketBus.publish(Span<MessageIn>(msgBuffer));
    }
    if (tail_ + MAX_SERIALIZED_MESSAGE_SIZE * 5 > BUFFER_SIZE) {
      rotateBuffer();
    }
    asyncRead();
  }

  void rotateBuffer() {
    std::memmove(readBuffer_.data(), readBuffer_.data() + head_, tail_ - head_);
    tail_ = tail_ - head_;
    head_ = 0;
  }

private:
  Socket socket_;
  Bus &bus_;
  Endpoint endpoint_;

  size_t head_{0};
  size_t tail_{0};
  ByteBuffer readBuffer_;
  TraderId traderId_{0};

  std::atomic<SocketStatus> status_{SocketStatus::Disconnected};
};

} // namespace hft

#endif // HFT_COMMON_ASYNCSOCKET_HPP
