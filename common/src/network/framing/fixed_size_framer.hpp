/**
 * @author Vladimir Pavliv
 * @date 2025-03-29
 */

#ifndef HFT_COMMON_FIXEDSIZEFRAMER_HPP
#define HFT_COMMON_FIXEDSIZEFRAMER_HPP

#include "boost_types.hpp"
#include "concepts/busable.hpp"
#include "constants.hpp"
#include "logging.hpp"
#include "serialization/flat_buffers/fbs_domain_serializer.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft {

/**
 * @brief Frames messages by writing message size and then serialized data
 */
template <typename SerializerType = serialization::FbsDomainSerializer>
class FixedSizeFramer {
public:
  using Serializer = SerializerType;

  template <typename EventType>
  static constexpr bool Framable = Serializer::template Serializable<EventType>;

  static constexpr size_t HEADER_SIZE = sizeof(MessageSize);

  template <typename Type>
  static void frame(CRef<Type> message, ByteBuffer &buffer) {
    const auto serializedMsg = Serializer::serialize(message);
    buffer.resize(serializedMsg.size() + HEADER_SIZE);

    const LittleEndianUInt16 bodySize = static_cast<MessageSize>(serializedMsg.size());
    std::memcpy(buffer.data(), &bodySize, sizeof(bodySize));
    std::memcpy(buffer.data() + sizeof(bodySize), serializedMsg.data(), serializedMsg.size());
  }

  template <Busable Consumer>
  static auto unframe(ByteSpan dataBuffer, Consumer &consumer) -> Expected<size_t> {
    const auto dataPtr = dataBuffer.data();
    size_t cursor{0};

    while (cursor + HEADER_SIZE <= dataBuffer.size()) {
      const auto lilBodySize = *reinterpret_cast<const LittleEndianUInt16 *>(dataPtr + cursor);
      const MessageSize bodySize = lilBodySize.value();
      LOG_TRACE("Trying to parse {} bytes", bodySize);
      // Check if there is enough data to deserialize message
      if (cursor + HEADER_SIZE + bodySize > dataBuffer.size()) {
        break;
      }
      if (!Serializer::deserialize(dataPtr + cursor + HEADER_SIZE, bodySize, consumer)) {
        return std::unexpected(StatusCode::Error);
      }
      cursor += bodySize + HEADER_SIZE;
    }
    return cursor;
  }
};

} // namespace hft

#endif // HFT_COMMON_FIXEDSIZEFRAMER_HPP
