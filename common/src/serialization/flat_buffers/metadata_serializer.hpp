/**
 * @author Vladimir Pavliv
 * @date 2025-04-08
 */

#ifndef HFT_COMMON_SERIALIZATION_METADATASERIALIZER_HPP
#define HFT_COMMON_SERIALIZATION_METADATASERIALIZER_HPP

#include "converter.hpp"
#include "gen/metadata_generated.h"
#include "metadata_types.hpp"
#include "template_types.hpp"

namespace hft::serialization::fbs {

class MetadataSerializer {
public:
  using BufferType = flatbuffers::DetachedBuffer;
  using SupportedTypes = std::tuple<OrderTimestamp>;
  using RootMessage = gen::fbs::meta::OrderTimestamp;

  template <typename EventType>
  static constexpr bool Serializable = IsTypeInTuple<EventType, SupportedTypes>;

  template <typename Consumer>
  static bool deserialize(const uint8_t *data, size_t size, Consumer &&consumer) {
    if (!flatbuffers::Verifier(data, size).VerifyBuffer<RootMessage>()) {
      LOG_ERROR("Failed to verify Buffer");
      return false;
    }
    const auto msg = flatbuffers::GetRoot<RootMessage>(data);
    if (msg == nullptr) {
      LOG_ERROR("Failed to extract Message");
      return false;
    }
    consumer.post(OrderTimestamp{msg->order_id(), msg->timestamp(), convert(msg->event_type())});
    return true;
  }

  static BufferType serialize(CRef<OrderTimestamp> event) {
    LOG_DEBUG("Serializing {}", utils::toString(event));
    using namespace gen::fbs::meta;
    flatbuffers::FlatBufferBuilder builder;
    const auto msg =
        CreateOrderTimestamp(builder, event.orderId, event.timestamp, convert(event.type));
    builder.Finish(msg);
    return builder.Release();
  }
};

} // namespace hft::serialization::fbs

#endif // HFT_COMMON_SERIALIZATION_METADATASERIALIZER_HPP
