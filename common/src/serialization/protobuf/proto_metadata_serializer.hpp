/**
 * @author Vladimir Pavliv
 * @date 2025-04-19
 */

#ifndef HFT_COMMON_SERIALIZATION_PROTOMETADATASERIALIZER_HPP
#define HFT_COMMON_SERIALIZATION_PROTOMETADATASERIALIZER_HPP

#include <concepts>

#include "gen/proto/metadata_messages.pb.h"
#include "metadata_types.hpp"
#include "types.hpp"

namespace hft::serialization {

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

template <typename MessageType>
concept ProtoSerializableType =
    std::same_as<MessageType, OrderTimestamp> || std::same_as<MessageType, RuntimeMetrics>;

template <typename MessageType>
concept ProtoDeserializableType = // format
    std::same_as<MessageType, gen::proto::metadata::OrderTimestamp> ||
    std::same_as<MessageType, gen::proto::metadata::RuntimeMetrics>;

class ProtoMetadataSerializer {
public:
  static String serialize(CRef<OrderTimestamp> msg) {
    using namespace google::protobuf::io;

    gen::proto::metadata::OrderTimestamp protoMsg;

    protoMsg.set_order_id(msg.orderId);
    protoMsg.set_created(msg.created);
    protoMsg.set_fulfilled(msg.fulfilled);
    protoMsg.set_notified(msg.notified);

    return frame<gen::proto::metadata::OrderTimestamp>(protoMsg);
  }

  static String serialize(CRef<RuntimeMetrics> msg) {
    using namespace google::protobuf::io;

    gen::proto::metadata::RuntimeMetrics protoMsg;

    gen::proto::metadata::Source source{gen::proto::metadata::Source::UNKNOWN};
    switch (msg.source) {
    case RuntimeMetrics::Source::Client:
      source = gen::proto::metadata::Source::CLIENT;
      break;
    case RuntimeMetrics::Source::Server:
      source = gen::proto::metadata::Source::SERVER;
      break;
    }

    protoMsg.set_source(source);
    protoMsg.set_timestamp_us(utils::getTimestamp());
    protoMsg.set_rps(msg.rps);
    protoMsg.set_avg_latency_us(msg.avgLatencyUs);

    return frame<gen::proto::metadata::RuntimeMetrics>(protoMsg);
  }

  template <ProtoDeserializableType MessageType>
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

  template <ProtoSerializableType MessageType, typename Consumer>
  static bool deserialize(const uint8_t *data, size_t size, Consumer &&consumer) {
    using namespace google::protobuf::io;

    ArrayInputStream arrayInputStream(data, static_cast<int>(size));
    CodedInputStream codedInputStream(&arrayInputStream);

    uint32_t messageLength = 0;
    if (!codedInputStream.ReadVarint32(&messageLength)) {
      LOG_ERROR("Failed to read message length varint");
      return false;
    }

    CodedInputStream::Limit msgLimit = codedInputStream.PushLimit(messageLength);

    MessageType protoMsg;
    if (!protoMsg.ParseFromCodedStream(&codedInputStream) ||
        !codedInputStream.ConsumedEntireMessage()) {
      LOG_ERROR("Failed to parse delimited protobuf message");
      return false;
    }

    codedInputStream.PopLimit(msgLimit);

    if constexpr (std::same_as<MessageType, OrderTimestamp>) {
      OrderTimestamp stamp;
      stamp.orderId = protoMsg.order_id();
      stamp.created = protoMsg.created();
      stamp.fulfilled = protoMsg.fulfilled();
      stamp.notified = protoMsg.notified();

      consumer.post(stamp);
    } else if constexpr (std::same_as<MessageType, RuntimeMetrics>) {
      RuntimeMetrics metrics;
      switch (protoMsg.source()) {
      case gen::proto::metadata::Source::CLIENT:
        metrics.source = RuntimeMetrics::Source::Client;
        break;
      case gen::proto::metadata::Source::SERVER:
        metrics.source = RuntimeMetrics::Source::Server;
        break;
      default:
        metrics.source = RuntimeMetrics::Source::Unknown;
        break;
      }
      metrics.rps = protoMsg.rps();
      metrics.timeStamp = protoMsg.timestamp_us();
      metrics.avgLatencyUs = protoMsg.avg_latency();

      consumer.post(metrics);
    }

    return true;
  }
};

} // namespace hft::serialization

#endif // HFT_COMMON_SERIALIZATION_PROTOMETADATASERIALIZER_HPP
