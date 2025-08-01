/**
 * @author Vladimir Pavliv
 * @date 2025-04-08
 */

#ifndef HFT_COMMON_METADATASERIALIZER_HPP
#define HFT_COMMON_METADATASERIALIZER_HPP

#include "concepts/busable.hpp"
#include "fbs_converter.hpp"
#include "gen/fbs/metadata_messages_generated.h"
#include "metadata_types.hpp"

namespace hft::serialization {

class FbsMetadataSerializer {
public:
  using BufferType = flatbuffers::DetachedBuffer;
  using SupportedTypes = std::tuple<OrderTimestamp>;
  using RootMessage = gen::fbs::metadata::OrderTimestamp;

  template <typename EventType>
  static constexpr bool Serializable = IsTypeInTuple<EventType, SupportedTypes>;

  template <Busable Consumer>
  static bool deserialize(const uint8_t *data, size_t size, Consumer &consumer) {
    if (!flatbuffers::Verifier(data, size).VerifyBuffer<RootMessage>()) {
      LOG_ERROR("Failed to verify Buffer");
      return false;
    }
    const auto msg = flatbuffers::GetRoot<RootMessage>(data);
    if (msg == nullptr) {
      LOG_ERROR("Failed to extract Message");
      return false;
    }
    consumer.post(
        OrderTimestamp{msg->order_id(), msg->created(), msg->accepted(), msg->notified()});
    return true;
  }

  static BufferType serialize(CRef<OrderTimestamp> event) {
    LOG_DEBUG("Serializing {}", utils::toString(event));
    using namespace gen::fbs::metadata;
    flatbuffers::FlatBufferBuilder builder;
    const auto msg =
        CreateOrderTimestamp(builder, event.orderId, event.created, event.accepted, event.notified);
    builder.Finish(msg);
    return builder.Release();
  }
};

} // namespace hft::serialization

#endif // HFT_COMMON_METADATASERIALIZER_HPP
