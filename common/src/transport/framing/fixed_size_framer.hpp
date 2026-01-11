/**
 * @author Vladimir Pavliv
 * @date 2025-03-29
 */

#ifndef HFT_COMMON_FIXEDSIZEFRAMER_HPP
#define HFT_COMMON_FIXEDSIZEFRAMER_HPP

#include "bus/busable.hpp"
#include "constants.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"
#include "utils/binary_utils.hpp"
#include "utils/string_utils.hpp"

namespace hft {

/**
 * @brief Frames messages by writing message size and then serialized data
 */
template <typename SerializerType>
class FixedSizeFramer {
public:
  using Serializer = SerializerType;

  template <typename EventType>
  static constexpr bool Framable = Serializer::template Serializable<EventType>;

  static constexpr size_t HEADER_SIZE = sizeof(MessageSize);

  template <typename Type>
  static size_t frame(CRef<Type> message, uint8_t *buffer) {
    const auto msgSize = Serializer::serialize(message, buffer + HEADER_SIZE);

    const utils::LittleEndianUInt16 bodySize = static_cast<MessageSize>(msgSize);
    std::memcpy(buffer, &bodySize, sizeof(bodySize));
    return HEADER_SIZE + msgSize;
  }

  static auto unframe(ByteSpan dataBuffer, Busable auto &consumer) -> Expected<size_t> {
    LOG_DEBUG("unframe {} bytes", dataBuffer.size());

    const auto dataPtr = dataBuffer.data();
    size_t cursor{0};

    while (cursor + HEADER_SIZE < dataBuffer.size()) {
      const auto lilBodySize =
          *reinterpret_cast<const utils::LittleEndianUInt16 *>(dataPtr + cursor);
      const MessageSize bodySize = lilBodySize.value();
      LOG_DEBUG("Trying to parse {} bytes", bodySize);
      // Check if there is enough data to deserialize message
      if (bodySize == 0 || bodySize > 1024) {
        LOG_ERROR("Invalid body size {}", bodySize);
        return std::unexpected(StatusCode::Error);
      }
      if (cursor + HEADER_SIZE + bodySize > dataBuffer.size()) {
        break;
      }
      auto msgSize = Serializer::deserialize(dataPtr + cursor + HEADER_SIZE, bodySize, consumer);
      if (!msgSize) {
        return msgSize;
      }
      if (*msgSize != bodySize) {
        return std::unexpected(StatusCode::Error);
      }
      LOG_DEBUG("unframed {}", bodySize + HEADER_SIZE);
      cursor += bodySize + HEADER_SIZE;
    }
    return cursor;
  }
};

} // namespace hft

#endif // HFT_COMMON_FIXEDSIZEFRAMER_HPP
