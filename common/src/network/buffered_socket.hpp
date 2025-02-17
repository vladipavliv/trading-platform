/**
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_BUFFEREDSOCKET_HPP
#define HFT_COMMON_BUFFEREDSOCKET_HPP

#include <memory>
#include <spdlog/spdlog.h>
#include <vector>

#include "network_types.hpp"
#include "types.hpp"

namespace hft {

// TODO(do): Enable raw sockets timestamping
/**
 */
template <typename SinkType, typename SerializerType, typename SocketType, typename MessageTypeIn>
class BufferedSocket {
public:
  using Type = BufferedSocket<SinkType, SerializerType, SocketType, MessageTypeIn>;
  using Sink = SinkType;
  using Serializer = SerializerType;
  using Socket = SocketType;
  using Endpoint = SocketType::endpoint_type;
  using MessageIn = MessageTypeIn;
  using UPtr = std::unique_ptr<Type>;

  BufferedSocket(Sink &sink, Socket &&socket)
      : mSink{sink}, mSocket{std::move(socket)}, mBuffer(BUFFER_SIZE),
        mId{utils::getTraderId(mSocket)} {
    if constexpr (std::is_same_v<Socket, UdpSocket>) {
      mSocket.set_option(boost::asio::socket_base::reuse_address{true});
    }
  }

  BufferedSocket(Sink &sink, Socket &&socket, Endpoint endpoint)
      : mSink{sink}, mSocket{std::move(socket)}, mEndpoint{std::move(endpoint)},
        mBuffer(BUFFER_SIZE) {
    if constexpr (std::is_same_v<Socket, UdpSocket>) {
      mSocket.set_option(boost::asio::socket_base::reuse_address{true});
    }
  }

  void asyncConnect() {
    if constexpr (std::is_same_v<Socket, TcpSocket>) {
      mSocket.async_connect(mEndpoint, [this](BoostErrorRef ec) { connectHandler(ec); });
    } else if constexpr (std::is_same_v<Socket, UdpSocket>) {
      asyncRead();
    }
  }

  void asyncRead() {
    size_t writable = mBuffer.size() - mTail;
    uint8_t *writePtr = mBuffer.data() + mHead;

    if constexpr (std::is_same_v<Socket, TcpSocket>) {
      mSocket.async_read_some(
          boost::asio::buffer(writePtr, writable),
          [this](BoostErrorRef code, size_t bytesRead) { readHandler(code, bytesRead); });
    } else if constexpr (std::is_same_v<Socket, UdpSocket>) {
      mSocket.async_receive_from(
          boost::asio::buffer(writePtr, writable), mEndpoint,
          [this](BoostErrorRef code, size_t bytesRead) { readHandler(code, bytesRead); });
    }
  }

  template <typename MessageTypeOut>
  void asyncWrite(const MessageTypeOut &msg) {
    if (!mSocket.is_open()) {
      spdlog::error("Failed to write to the socket: not opened");
      return;
    }
    auto buffer = Serializer::serialize(msg);
    MessageSize bodySize = htons(static_cast<MessageSize>(buffer.size()));
    // Could reuse those buffers
    auto dataPtr = std::make_shared<ByteBuffer>(sizeof(MessageSize) + buffer.size());

    std::memcpy(dataPtr->data(), &bodySize, sizeof(bodySize));
    std::memcpy(dataPtr->data() + sizeof(bodySize), buffer.data(), buffer.size());

    if constexpr (std::is_same_v<Socket, TcpSocket>) {
      boost::asio::async_write(
          mSocket, boost::asio::buffer(dataPtr->data(), dataPtr->size()),
          [this, dataPtr](BoostErrorRef ec, size_t size) { writeHandler(ec, size); });
    } else if constexpr (std::is_same_v<Socket, UdpSocket>) {
      mSocket.async_send_to(
          boost::asio::buffer(dataPtr->data(), dataPtr->size()), mEndpoint,
          [this, dataPtr](BoostErrorRef ec, size_t size) { writeHandler(ec, size); });
    }
  }

  template <typename MessageTypeOut>
  void asyncWrite(const std::vector<MessageTypeOut> &msgVec) {
    if (!mSocket.is_open()) {
      spdlog::error("Failed to write to the socket: not opened");
      return;
    }
    size_t allocSize = msgVec.size() * MAX_MESSAGE_SIZE;
    // Could reuse those buffers
    auto dataPtr = std::make_shared<ByteBuffer>(allocSize);

    size_t realSize{0};
    uint8_t *cursor = dataPtr->data();
    for (auto &msg : msgVec) {
      auto buffer = Serializer::serialize(msg);
      MessageSize bodySize = htons(static_cast<MessageSize>(buffer.size()));

      std::memcpy(cursor, &bodySize, sizeof(bodySize));
      std::memcpy(cursor + sizeof(bodySize), buffer.data(), buffer.size());
      cursor += bodySize + sizeof(bodySize);
      realSize += bodySize + sizeof(bodySize);
    }

    if constexpr (std::is_same_v<Socket, TcpSocket>) {
      boost::asio::async_write(
          mSocket, boost::asio::buffer(dataPtr->data(), dataPtr->size()),
          [this, dataPtr](BoostErrorRef ec, size_t size) { writeHandler(ec, size); });
    } else if constexpr (std::is_same_v<Socket, UdpSocket>) {
      mSocket.async_send_to(
          boost::asio::buffer(dataPtr->data(), dataPtr->size()), mEndpoint,
          [this, dataPtr](BoostErrorRef ec, size_t size) { writeHandler(ec, size); });
    }
  }

  void close() { mSocket.close(); }

  void retrieveTraderId() {
    if constexpr (std::is_same_v<Socket, TcpSocket>) {
      if (mId == std::numeric_limits<TraderId>::max()) {
        mId = utils::getTraderId(mSocket);
      }
    }
  }

  inline TraderId getTraderId() const { return mId; }

private:
  void readHandler(BoostErrorRef ec, size_t bytesRead) {
    if (ec) {
      if (ec != boost::asio::error::eof) {
        spdlog::error("Failed to read from the socket: {}", ec.message());
      }
      mHead = mTail = 0;
      return;
    }
    if (bytesRead > mBuffer.size()) {
      spdlog::error("Socket read too much data");
      mHead = mTail = 0;
      return;
    }
    if (mTail + bytesRead > mBuffer.size()) {
      spdlog::error("Socket tail out of bounds");
      mHead = mTail = 0;
      return;
    }
    mTail += bytesRead;
    // spdlog::trace("Read {} bytes mHead {} mTail {}", bytesRead, mHead, mTail);

    uint8_t *cursor = mBuffer.data() + mHead;
    std::vector<MessageIn> messages;
    while (mHead + sizeof(MessageSize) < mTail) {
      MessageSize bodySize{0};
      std::memcpy(&bodySize, cursor, sizeof(MessageSize));
      bodySize = ntohs(bodySize);
      if (bodySize > mBuffer.size()) {
        spdlog::error("Invalid body size {}", bodySize);
        mHead = mTail = 0;
        break;
      }
      if (mHead + bodySize + sizeof(MessageSize) > mTail) {
        // Unable to parse message, wait for more data
        break;
      }
      cursor += sizeof(MessageSize);
      auto result = Serializer::template deserialize<MessageIn>(cursor, bodySize);
      if (!result.ok()) {
        spdlog::error("Message parsing failed");
        mHead = mTail = 0;
        break;
      }
      auto message = result.value();
      if constexpr (!std::is_same_v<MessageTypeIn, TickerPrice>) {
        message.traderId = getTraderId();
      }
      messages.emplace_back(std::move(message));
      mHead += bodySize + sizeof(MessageSize);
      if (mHead > mTail) {
        spdlog::error("mHead > mTail");
        mHead = mTail = 0;
        break;
      }
    }
    // If we getting close to the edge of the buffer lets just wrap around
    if (mBuffer.size() - mTail < 256) {
      // memmove handles potential overlap fine, but it should not happen here anyway
      std::memmove(mBuffer.data(), mBuffer.data() + mHead, mTail - mHead);
      mTail = mTail - mHead;
      mHead = 0;
    }
    if (!messages.empty()) {
      mSink.dataSink.post(messages);
    }
    asyncRead();
  }

  void connectHandler(BoostErrorRef ec) {
    if (ec) {
      spdlog::error("Connect failed: {}", ec.message());
      return;
    }
    mSocket.set_option(TcpSocket::protocol_type::no_delay(true));
    asyncRead();
  }

  void writeHandler(BoostErrorRef ec, size_t written) {
    if (ec) {
      spdlog::error("Write failed: {}", ec.message());
    }
  }

private:
  Sink &mSink;
  Socket mSocket;
  Endpoint mEndpoint;

  size_t mHead{0};
  size_t mTail{0};
  ByteBuffer mBuffer;

  TraderId mId{std::numeric_limits<TraderId>::max()};
};

} // namespace hft

#endif // HFT_COMMON_BUFFEREDSOCKET_HPP
