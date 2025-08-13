/**
 * @author Vladimir Pavliv
 * @date 2025-03-29
 */

#ifndef HFT_COMMON_DUMMYFRAMER_HPP
#define HFT_COMMON_DUMMYFRAMER_HPP

#include "boost_types.hpp"
#include "concepts/busable.hpp"
#include "constants.hpp"
#include "logging.hpp"
#include "serialization/serializer.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft {

/**
 * @brief Does not performa any framing
 * @details Usefull when serializer does framing automatically, or the size of all messages is fixed
 * @todo Unify serializers interfaces
 */
template <typename SerializerType = serialization::DomainSerializer>
class DummyFramer {
public:
  using Serializer = SerializerType;

  template <typename EventType>
  static constexpr bool Framable = Serializer::template Serializable<EventType>;

  template <typename Type>
  static void frame(CRef<Type> message, ByteBuffer &buffer) {
    Serializer::serialize(message, buffer);
  }

  template <Busable Bus>
  static auto unframe(ByteSpan buff, Bus &bus) -> Expected<size_t> {
    size_t processedSize{0};
    size_t msgSize{0};
    do {
      const auto msgPtr = buff.data() + processedSize;
      const auto remainSize = buff.size() - processedSize;
      msgSize = Serializer::deserialize(msgPtr, remainSize, bus);
      processedSize += msgSize;
    } while (msgSize != 0);
    return processedSize;
  }
};

} // namespace hft

#endif // HFT_COMMON_DUMMYFRAMER_HPP
