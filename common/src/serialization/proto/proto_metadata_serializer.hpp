/**
 * @author Vladimir Pavliv
 * @date 2025-04-19
 */

#ifndef HFT_COMMON_SERIALIZATION_PROTOMETADATASERIALIZER_HPP
#define HFT_COMMON_SERIALIZATION_PROTOMETADATASERIALIZER_HPP

#include "bus/busable.hpp"
#include "metadata_types.hpp"
#include "primitive_types.hpp"
#include "proto/cpp/metadata_messages.pb.h"
#include "proto_converter.hpp"
#include "utils/trait_utils.hpp"

namespace hft::serialization::proto {

/**
 * @brief Protobuf serializer
 * @details ClickHouse doesn't support flatbuffers for direct consuming from kafka
 * protobuf is slightly slower then flatbuffers, in theory a monitor service could
 * consume kafka messages, deserialize them and then insert the data to ClickHouse
 * which would work with fbs serialization, but connecting Kafka to ClickHouse
 * directly is convenient, and almost certainly would end up even more efficient
 * @todo ClickHouse requires messages to be framed with varint, so a quick framing
 * was made here directly, need to split it to separate VarintSizeFramer
 */
class ProtoMetadataSerializer {
public:
  using SupportedTypes = std::tuple<OrderTimestamp, RuntimeMetrics>;

  template <typename EventType>
  static constexpr bool Serializable = utils::IsTypeInTuple<EventType, SupportedTypes>;

  static String serialize(CRef<OrderTimestamp> msg) {
    using namespace google::protobuf::io;

    gen::proto::metadata::Envelope envelope;
    auto *protoMsg = envelope.mutable_order_timestamp();

    protoMsg->set_order_id(msg.orderId);
    protoMsg->set_created(msg.created);
    protoMsg->set_fulfilled(msg.fulfilled);
    protoMsg->set_notified(msg.notified);

    return frame<gen::proto::metadata::Envelope>(envelope);
  }

  static String serialize(CRef<RuntimeMetrics> msg) {
    using namespace google::protobuf::io;

    gen::proto::metadata::Envelope envelope;
    auto *protoMsg = envelope.mutable_runtime_metrics();

    protoMsg->set_source(convert(msg.source));
    protoMsg->set_timestamp_us(utils::getTimestampNs());
    protoMsg->set_rps(msg.rps);
    protoMsg->set_avg_latency_us(msg.avgLatencyUs);

    return frame<gen::proto::metadata::Envelope>(envelope);
  }

  template <typename MessageType>
  static String frame(CRef<MessageType> message) {
    using namespace google::protobuf::io;

    String serializedMsg;
    if (!message.SerializeToString(&serializedMsg)) {
      LOG_ERROR("Failed to serialize OrderTimestamp");
      serializedMsg.clear();
    }
    String result;

    StringOutputStream outputStream(&result);
    CodedOutputStream codedOutput(&outputStream);

    codedOutput.WriteVarint32(serializedMsg.size());
    codedOutput.WriteRaw(serializedMsg.data(), serializedMsg.size());

    return result;
  }

  static bool deserialize(const uint8_t *data, size_t size, Busable auto &consumer) {
    using namespace google::protobuf::io;

    ArrayInputStream arrayInputStream(data, static_cast<int>(size));
    CodedInputStream codedInputStream(&arrayInputStream);

    uint32_t messageLength = 0;
    if (!codedInputStream.ReadVarint32(&messageLength)) {
      LOG_ERROR("Failed to read message length varint");
      return false;
    }

    CodedInputStream::Limit msgLimit = codedInputStream.PushLimit(messageLength);

    gen::proto::metadata::Envelope envelope;
    if (!envelope.ParseFromCodedStream(&codedInputStream) ||
        !codedInputStream.ConsumedEntireMessage()) {
      LOG_ERROR("Failed to parse delimited protobuf message");
      return false;
    }

    codedInputStream.PopLimit(msgLimit);

    switch (envelope.payload_case()) {
    case gen::proto::metadata::Envelope::kOrderTimestamp: {
      const auto &protoMsg = envelope.order_timestamp();
      OrderTimestamp stamp;

      stamp.orderId = protoMsg.order_id();
      stamp.created = protoMsg.created();
      stamp.fulfilled = protoMsg.fulfilled();
      stamp.notified = protoMsg.notified();

      LOG_TRACE("{}", toString(stamp));

      consumer.post(stamp);
      break;
    }
    case gen::proto::metadata::Envelope::kRuntimeMetrics: {
      const auto &protoMsg = envelope.runtime_metrics();
      RuntimeMetrics metrics;

      metrics.source = convert(protoMsg.source());
      metrics.rps = protoMsg.rps();
      metrics.timeStamp = protoMsg.timestamp_us();
      metrics.avgLatencyUs = protoMsg.avg_latency_us();

      LOG_TRACE("{}", toString(metrics));

      consumer.post(metrics);
      break;
    }
    case gen::proto::metadata::Envelope::kLogEntry: {
      const auto &protoMsg = envelope.log_entry();
      LogEntry entry;

      entry.source = convert(protoMsg.source());
      entry.message = protoMsg.message();
      entry.level = protoMsg.level();

      LOG_TRACE("{}", toString(entry));

      consumer.post(entry);
      break;
    }
    case gen::proto::metadata::Envelope::PAYLOAD_NOT_SET:
    default:
      LOG_ERROR("Envelope has no recognized payload");
      return false;
    }
    return true;
  }
};

} // namespace hft::serialization::proto

#endif // HFT_COMMON_SERIALIZATION_PROTOMETADATASERIALIZER_HPP
