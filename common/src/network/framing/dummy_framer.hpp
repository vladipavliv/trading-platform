/**
 * @author Vladimir Pavliv
 * @date 2025-03-29
 */

#ifndef HFT_COMMON_DUMMYFRAMER_HPP
#define HFT_COMMON_DUMMYFRAMER_HPP

#include "boost_types.hpp"
#include "bus/busable.hpp"
#include "constants.hpp"
#include "logging.hpp"
#include "types.hpp"
#include "utils/string_utils.hpp"

namespace hft {

/**
 * @brief Does not performa any framing
 * @details Usefull when serializer does framing automatically, or the size of all messages is fixed
 * @todo Unify serializers interfaces
 */
template <typename Serializer>
class DummyFramer {
public:
  template <typename EventType>
  static constexpr bool Framable = Serializer::template Serializable<EventType>;

  template <typename Type>
  static size_t frame(CRef<Type> message, uint8_t *buffer) {
    return Serializer::serialize(message, buffer);
  }

  static auto unframe(ByteSpan buff, Busable auto &bus) -> Expected<size_t> {
    size_t processedSize{0};
    while (processedSize < buff.size()) {
      const auto msgPtr = buff.data() + processedSize;
      const auto remainSize = buff.size() - processedSize;
      auto msgSize = Serializer::deserialize(msgPtr, remainSize, bus);
      if (!msgSize) {
        return msgSize;
      }
      processedSize += *msgSize;
    }
    return processedSize;
  }
};

} // namespace hft

#endif // HFT_COMMON_DUMMYFRAMER_HPP
