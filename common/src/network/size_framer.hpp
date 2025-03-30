/**
 * @author Vladimir Pavliv
 * @date 2025-03-29
 */

#ifndef HFT_COMMON_SIZEFRAMER_HPP
#define HFT_COMMON_SIZEFRAMER_HPP

#include <ranges>

#include "boost_types.hpp"
#include "bus/bus.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include "ring_buffer.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft {

template <typename SerializerType>
class SizeFramer {
public:
  using Serializer = SerializerType;

  static constexpr size_t HEADER_SIZE = sizeof(MessageSize);

  template <typename Type>
  static SPtr<ByteBuffer> frame(Span<Type> messages) {
    spdlog::trace("frame {} messages", messages.size());
    std::vector<typename Serializer::BufferType> serializedMessages(messages.size());

    size_t allocSize{0};
    for (auto [index, message] : messages | std::views::enumerate) {
      serializedMessages[index] = Serializer::serialize(message);
      allocSize += sizeof(MessageSize) + serializedMessages[index].size();
    }

    SPtr<ByteBuffer> writeBuffer = std::make_shared<ByteBuffer>(allocSize);
    auto cursor = writeBuffer->data();
    for (auto [index, message] : serializedMessages | std::views::enumerate) {
      LittleEndianUInt16 bodySize = static_cast<MessageSize>(message.size());
      std::memcpy(cursor, &bodySize, sizeof(bodySize));
      std::memcpy(cursor + sizeof(bodySize), message.data(), message.size());
      cursor += sizeof(bodySize) + bodySize;
    }
    return writeBuffer;
  }

  template <typename Type>
  static SPtr<ByteBuffer> frame(CRef<Type> message) {
    spdlog::trace("frame 1 message");

    auto serialized = Serializer::serialize(message);
    SPtr<ByteBuffer> buffer = std::make_shared<ByteBuffer>(sizeof(MessageSize) + serialized.size());
    LittleEndianUInt16 bodySize = static_cast<MessageSize>(serialized.size());

    std::memcpy(buffer->data(), &bodySize, sizeof(bodySize));
    std::memcpy(buffer->data() + sizeof(bodySize), serialized.data(), serialized.size());

    return buffer;
  }

  template <typename Cunsumer>
  static void unframe(RingBuffer &buffer, Cunsumer &consumer) {
    // TODO(self) Try some buckets here automatically sorting messages by type and workerId
    auto readBuffer = buffer.data();
    auto dataPtr = static_cast<const uint8_t *>(readBuffer.data());
    auto cursor{0};

    spdlog::trace("unframe buffer size {} ", readBuffer.size());

    while (cursor + HEADER_SIZE <= readBuffer.size()) {
      auto littleBodySize = *reinterpret_cast<const LittleEndianUInt16 *>(dataPtr + cursor);
      MessageSize bodySize = littleBodySize.value();

      if (cursor + HEADER_SIZE + bodySize > readBuffer.size()) {
        break;
      }
      cursor += HEADER_SIZE;
      if (!Serializer::deserialize(dataPtr + cursor, bodySize, consumer)) {
        buffer.reset();
        return;
      }
      buffer.commitRead(HEADER_SIZE + bodySize);
      cursor += bodySize;
    }
  }
};

} // namespace hft

#endif // HFT_COMMON_SIZEFRAMER_HPP