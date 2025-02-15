/**
 * @file
 * @brief
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

  BufferedSocket(Sink &sink)
      : mSink{sink}, mSocket{mSink.networkSink.ctx()}, mTimer{mSink.networkSink.ctx()} {
    mBuffer.reserve(BUFFER_SIZE);
  } // TODO(do)

  BufferedSocket(Sink &sink, Socket &&socket)
      : mSink{sink}, mSocket{std::move(socket)}, mTimer{mSink.networkSink.ctx()} {
    mBuffer.reserve(BUFFER_SIZE);
  }

  void asyncConnect(Endpoint &&endpoint) {
    mEndpoint = std::move(endpoint);
    asyncConnect();
  }

  void asyncRead() {
    size_t writable = BUFFER_SIZE - mTail;
    uint8_t *writePtr = mBuffer.data() + mHead;

    mSocket.async_read_some(
        boost::asio::buffer(writePtr, writable),
        [this](BoostErrorRef code, size_t bytesRead) { readHandler(code, bytesRead); });
  }

  template <typename MessageTypeOut>
  void asyncWrite(const MessageTypeOut &msg) {
    if (!mSocket.is_open()) {
      spdlog::error("Failed to write to the socket: not opened");
      return;
    }
    auto buffer = Serializer::serialize(msg);
    MessageSize bodySize = htons(static_cast<MessageSize>(buffer.size()));
    auto dataPtr = std::make_shared<ByteBuffer>(sizeof(MessageSize) + buffer.size());

    std::memcpy(dataPtr->data(), &bodySize, sizeof(bodySize));
    std::memcpy(dataPtr->data() + sizeof(bodySize), buffer.data(), buffer.size());

    boost::asio::async_write(
        mSocket, boost::asio::buffer(dataPtr->data(), dataPtr->size()),
        [this, dataPtr](BoostErrorRef ec, size_t size) { writeHandler(ec, size); });
  }

  template <typename MessageTypeOut>
  void asyncWrite(const std::vector<MessageTypeOut> &msgVec) {
    if (!mSocket.is_open()) {
      spdlog::error("Failed to write to the socket: not opened");
      return;
    }
    size_t allocSize = msgVec.size() * MAX_MESSAGE_SIZE;
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

    boost::asio::async_write(
        mSocket, boost::asio::buffer(dataPtr->data(), realSize),
        [this, dataPtr](BoostErrorRef ec, size_t size) { writeHandler(ec, size); });
  }

  void close() { mSocket.close(); }

private:
  void readHandler(BoostErrorRef ec, size_t bytesRead) {
    if (ec) {
      spdlog::error("Failed to read from the socket: {}", ec.message());
      mHead = mTail = 0;
      return;
    }
    mTail += bytesRead;

    uint8_t *cursor = mBuffer.data();
    std::vector<MessageIn> messages;
    while (mHead + sizeof(MessageSize) < mTail) {
      MessageSize bodySize{0};
      std::memcpy(&bodySize, cursor + mHead, sizeof(MessageSize));
      bodySize = ntohs(bodySize);
      if (bodySize > BUFFER_SIZE) {
        spdlog::error("Invalid body size {}", bodySize);
        mHead = mTail = 0;
        break;
      }
      cursor += sizeof(MessageSize);
      if (mHead + bodySize + sizeof(MessageSize) > mTail) {
        // Unable to parse message, wait for more data
        break;
      }

      auto result = Serializer::template deserialize<MessageIn>(cursor, bodySize);
      if (!result.ok()) {
        // TODO(do) Clear all?
        mHead = mTail = 0;
        return;
      }
      auto message = result.extract();
      message.traderId = utils::getTraderId(mSocket);
      messages.emplace_back(std::move(message));
      mHead += bodySize + sizeof(MessageSize);
    }
    // If we getting close to the edge of the buffer lets just wrap around
    if (BUFFER_SIZE - mTail < 256) {
      // TODO(self): Would work for now but need improvement for overlap
      std::memcpy(mBuffer.data(), mBuffer.data() + mHead, mTail - mHead);
      mTail = mTail - mHead;
      mHead = 0;
    }
    if (!messages.empty()) {
      mSink.dataSink.post(messages);
    }
    asyncRead();
  }

  void asyncConnect() {
    if (!mEndpoint.has_value()) {
      spdlog::error("Socket ain't reconnectable");
      return;
    }
    mSocket.async_connect(mEndpoint.value(), [this](BoostErrorRef ec) { connectHandler(ec); });
  }

  void connectHandler(BoostErrorRef ec) {
    if (ec) {
      spdlog::error("Failed to connect: {}", ec.message());
      scheduleConnect();
    } else {
      spdlog::debug("Socket connected successfully");
      asyncRead();
    }
  }

  void scheduleConnect() {
    mTimer.expires_after(Seconds(3));
    mTimer.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        spdlog::error("Failed to reconnect");
        scheduleConnect();
      } else {
        asyncConnect();
      }
    });
  }

  void writeHandler(BoostErrorRef ec, size_t written) {
    if (ec) {
      spdlog::error("Failed to send message: {}", ec.message());
      scheduleConnect();
    }
  }

private:
  Sink &mSink;
  Socket mSocket;

  size_t mHead{0};
  size_t mTail{0};
  ByteBuffer mBuffer;

  SteadyTimer mTimer;
  std::optional<Endpoint> mEndpoint;
};

} // namespace hft

#endif // HFT_COMMON_BUFFEREDSOCKET_HPP
