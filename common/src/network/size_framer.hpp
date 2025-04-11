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
#include "logging.hpp"
#include "ring_buffer.hpp"
#include "serialization/flat_buffers/market_serializer.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft {

/**
 * @brief Frames messages by writing message size and then serialized data
 */
template <typename SerializerType = serialization::fbs::MarketSerializer>
class SizeFramer {
public:
  using Serializer = SerializerType;

  template <typename EventType>
  static constexpr bool Framable = Serializer::template Serializable<EventType>;

  static constexpr size_t HEADER_SIZE = sizeof(MessageSize);

  template <typename Type>
  static SPtr<ByteBuffer> frame(CRef<Type> message) {
    LOG_DEBUG("frame {}", utils::toString(message));

    const auto serializedMsg = Serializer::serialize(message);
    const auto buffer = std::make_shared<ByteBuffer>(sizeof(MessageSize) + serializedMsg.size());

    const LittleEndianUInt16 bodySize = static_cast<MessageSize>(serializedMsg.size());
    std::memcpy(buffer->data(), &bodySize, sizeof(bodySize));
    std::memcpy(buffer->data() + sizeof(bodySize), serializedMsg.data(), serializedMsg.size());

    return buffer;
  }

  template <typename Consumer>
  static void unframe(RingBuffer &buffer, Consumer &&consumer) {
    LOG_DEBUG("unframe");
    const auto readBuffer = buffer.data();
    const auto dataPtr = static_cast<const uint8_t *>(readBuffer.data());
    size_t cursor{0};

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