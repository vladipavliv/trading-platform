/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_RINGSOCKET_HPP
#define HFT_COMMON_RINGSOCKET_HPP

#include <memory>
#include <spdlog/spdlog.h>
#include <vector>

#include "network_types.hpp"
#include "types.hpp"

namespace hft {

template <typename SinkType, typename SerializerType, typename MessageIn, typename... MessagesOut>
class BufferedSocket {
public:
  using Sink = SinkType;
  using Serializer = SerializerType;
  using Message = MessageIn;
  using UPtr = std::unique_ptr<BufferedSocket<SinkType, SerializerType, MessageIn, MessagesOut...>>;

  BufferedSocket(Sink &sink, TcpSocket socket) : mSink{sink}, mSocket{std::move(socket)} {
    mBuffer.reserve(BUFFER_SIZE);
  }

  void asyncRead() {
    size_t writable = BUFFER_SIZE - mTail;
    uint8_t *writePtr = mBuffer.data() + mHead;

    mSocket.async_read_some(
        boost::asio::buffer(writePtr, writable),
        [this](BoostErrorRef code, size_t bytesRead) { readHandler(code, bytesRead); });
  }

  template <
      typename MessageType,
      typename = std::enable_if_t<(std::disjunction_v<std::is_same<MessageType, MessagesOut>...>)>>
  void write(MessageType &&msg) {
    auto buffer = Serializer::template serialize<MessageType>(std::forward<MessageType>(msg));
    MessageSize bodySize = htons(static_cast<MessageSize>(buffer.size()));
    auto dataPtr = std::make_shared<ByteBuffer>(sizeof(bodySize) + buffer.size());

    std::memcpy(dataPtr->data(), &bodySize, sizeof(bodySize));
    std::memcpy(dataPtr->data() + sizeof(bodySize), buffer.data(), buffer.size());

    boost::asio::async_write(
        mSocket, boost::asio::buffer(dataPtr->data(), dataPtr->size()),
        [this, dataPtr](BoostErrorRef ec, size_t size) { writeHandler(ec, size); });
  }

  template <
      typename MessageType,
      typename = std::enable_if_t<(std::disjunction_v<std::is_same<MessageType, MessagesOut>...>)>>
  void write(const std::vector<MessageType> &msgVec) {
    // Allocate approximately, to know exact size we need to serialize each message first
    size_t messageSize = msgVec.size() * MAX_MESSAGE_SIZE;
    auto dataPtr = std::make_shared<ByteBuffer>(messageSize);

    for (auto &msg : msgVec) {
      auto buffer = Serializer::serialize(msg);
      MessageSize bodySize = htons(static_cast<MessageSize>(buffer.size()));

      std::memcpy(dataPtr->data(), &bodySize, sizeof(bodySize));
      std::memcpy(dataPtr->data() + sizeof(bodySize), buffer.data(), buffer.size());
    }

    boost::asio::async_write(
        mSocket, boost::asio::buffer(dataPtr->data(), dataPtr->size()),
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
    std::vector<MessageIn> messages;
    while (mHead + sizeof(MessageSize) < mTail) {
      MessageSize bodySize{0};
      std::memcpy(&bodySize, mBuffer.data(), sizeof(MessageSize));
      bodySize = ntohs(bodySize);
      if (mHead + bodySize > mTail) {
        break;
      }
      auto result = Serializer::template deserialize<MessageIn>(mBuffer.data(),
                                                                bodySize + sizeof(MessageSize));
      if (!result.ok()) {
        // TODO(do) Clear all?
        mHead = mTail = 0;
        return;
      }
      messages.push_back(std::forward<MessageIn>(result.extract()));
      mHead += bodySize + sizeof(MessageSize);
    }
    // If we getting close to the edge of the buffer lets just wrap around
    if (BUFFER_SIZE - mTail < 256) {
      std::memcpy(mBuffer.data(), mBuffer.data() + mHead, mTail - mHead);
      mTail = mTail - mHead;
      mHead = 0;
    }
    if (!messages.empty()) {
      mSink.post(messages);
    }
    asyncRead();
  }

  void writeHandler(BoostErrorRef ec, size_t) {
    if (ec) {
      spdlog::error("Failed to send message: {}", ec.message());
      // reconnect();
    }
  }

private:
  Sink &mSink;
  TcpSocket mSocket;

  size_t mHead{0};
  size_t mTail{0};
  ByteBuffer mBuffer;
};

} // namespace hft

#endif // HFT_COMMON_RINGSOCKET_HPP