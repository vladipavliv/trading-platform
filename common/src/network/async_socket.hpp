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

#include "constants.hpp"
#include "network_types.hpp"
#include "pool/buffer_pool.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft {

/**
 * @brief Performs asynchronous reads and writes to a socket,
 */
template <typename SinkType, typename SerializerType, typename SocketType, typename MessageTypeIn>
class AsyncSocket {
public:
  using Type = AsyncSocket<SinkType, SerializerType, SocketType, MessageTypeIn>;
  using Sink = SinkType;
  using Serializer = SerializerType;
  using Socket = SocketType;
  using Endpoint = SocketType::endpoint_type;
  using MessageIn = MessageTypeIn;
  using UPtr = std::unique_ptr<Type>;

  AsyncSocket(Sink &sink, Socket &&socket)
      : mSink{sink}, mSocket{std::move(socket)}, mBuffer(BUFFER_SIZE),
        mId{utils::getTraderId(mSocket)} {
    mReadMsgBuffer.reserve(100);
    if constexpr (std::is_same_v<Socket, UdpSocket>) {
      mSocket.set_option(boost::asio::socket_base::reuse_address{true});
    }
  }

  AsyncSocket(Sink &sink, Socket &&socket, Endpoint endpoint)
      : mSink{sink}, mSocket{std::move(socket)}, mEndpoint{std::move(endpoint)},
        mBuffer(BUFFER_SIZE) {
    mReadMsgBuffer.reserve(100);
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
    uint8_t *writePtr = mBuffer.data() + mTail;

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
  void asyncWrite(Span<MessageTypeOut> msgVec) {
    if (!mSocket.is_open()) {
      spdlog::error("Failed to write to the socket: not opened");
      return;
    }

    size_t allocSize = msgVec.size() * MAX_SERIALIZED_MESSAGE_SIZE;
    auto dataPtr = mWritePool.acquire(allocSize);

    size_t totalSize{0};
    uint8_t *cursor = dataPtr;
    for (auto &msg : msgVec) {
      auto msgSize = serializeMessage(msg, cursor);
      cursor += msgSize;
      totalSize += msgSize;
    }

    if constexpr (std::is_same_v<Socket, TcpSocket>) {
      boost::asio::async_write(mSocket, boost::asio::buffer(dataPtr, totalSize),
                               [this, allocSize](BoostErrorRef ec, size_t size) {
                                 BufferGuard(mWritePool, allocSize);
                                 writeHandler(ec, size);
                               });
    } else if constexpr (std::is_same_v<Socket, UdpSocket>) {
      mSocket.async_send_to(boost::asio::buffer(dataPtr, totalSize), mEndpoint,
                            [this, allocSize](BoostErrorRef ec, size_t size) {
                              BufferGuard(mWritePool, allocSize);
                              writeHandler(ec, size);
                            });
    }
  }

  void retrieveTraderId() {
    if constexpr (std::is_same_v<Socket, TcpSocket>) {
      if (mId == 0) {
        mId = utils::getTraderId(mSocket);
      }
    }
  }

  /**
   * @brief Just a quick way to distinguish between clients
   */
  inline TraderId getTraderId() const { return mId; }

private:
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
      mHead = mTail = 0;
      if (ec != boost::asio::error::eof) {
        spdlog::error(ec.message());
      }
      return;
    }
    mTail += bytesRead;
    while (mHead + sizeof(MessageSize) < mTail) {
      uint8_t *cursor = mBuffer.data() + mHead;
      boost::endian::little_int16_at littleBodySize = 0;
      std::memcpy(&littleBodySize, cursor, sizeof(littleBodySize));
      MessageSize bodySize = littleBodySize.value();
      if (mHead + sizeof(MessageSize) + bodySize > mBuffer.size()) {
        rotateBuffer();
        break;
      }
      if (mHead + sizeof(MessageSize) + bodySize > mTail) {
        // continue reading;
        break;
      }
      cursor += sizeof(MessageSize);
      auto result = Serializer::template deserialize<MessageIn>(cursor, bodySize);
      if (!result.ok()) {
        mHead = mTail = 0;
        break;
      }
      mReadMsgBuffer.emplace_back(result.value());
      if constexpr (!std::is_same_v<MessageTypeIn, TickerPrice>) {
        mReadMsgBuffer.back().traderId = getTraderId();
      }
      mHead += bodySize + sizeof(MessageSize);
    }
    if (mBuffer.size() - mTail < 256) {
      rotateBuffer();
    }
    if (!mReadMsgBuffer.empty()) {
      mSink.dataSink.post(Span<MessageIn>(mReadMsgBuffer));
    }
    mReadMsgBuffer.clear();
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

  void rotateBuffer() {
    std::memmove(mBuffer.data(), mBuffer.data() + mHead, mTail - mHead);
    mTail = mTail - mHead;
    mHead = 0;
  }

private:
  Sink &mSink;
  Socket mSocket;
  Endpoint mEndpoint;

  size_t mHead{0};
  size_t mTail{0};
  ByteBuffer mBuffer;

  static thread_local BufferPool mWritePool;
  std::vector<MessageIn> mReadMsgBuffer;

  TraderId mId{};
};

template <typename SinkType, typename SerializerType, typename SocketType, typename MessageTypeIn>
thread_local BufferPool AsyncSocket<SinkType, SerializerType, SocketType,
                                    MessageTypeIn>::mWritePool{WRITE_BUFFER_POOL_SIZE};

} // namespace hft

#endif // HFT_COMMON_ASYNCSOCKET_HPP
