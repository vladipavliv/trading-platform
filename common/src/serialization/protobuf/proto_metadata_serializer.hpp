/**
 * @author Vladimir Pavliv
 * @date 2025-04-19
 */

#ifndef HFT_COMMON_SERIALIZATION_PROTOMETADATASERIALIZER_HPP
#define HFT_COMMON_SERIALIZATION_PROTOMETADATASERIALIZER_HPP

#include "gen/proto/metadata_messages.pb.h"
#include "metadata_types.hpp"
#include "types.hpp"

namespace hft::serialization {

class ProtoMetadataSerializer {
public:
  static String serialize(CRef<OrderTimestamp> msg) {
    using namespace google::protobuf::io;

    hft::serialization::gen::proto::metadata::OrderTimestamp protoMsg;

    protoMsg.set_order_id(msg.orderId);
    protoMsg.set_created(msg.created);
    protoMsg.set_fulfilled(msg.fulfilled);
    protoMsg.set_notified(msg.notified);

    String serializedMsg;
    if (!protoMsg.SerializeToString(&serializedMsg)) {
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

  template <typename Consumer>
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

    hft::serialization::gen::proto::metadata::OrderTimestamp protoMsg;
    if (!protoMsg.ParseFromCodedStream(&codedInputStream) ||
        !codedInputStream.ConsumedEntireMessage()) {
      LOG_ERROR("Failed to parse delimited protobuf message");
      return false;
    }

    codedInputStream.PopLimit(msgLimit);

    OrderTimestamp stamp;
    stamp.orderId = protoMsg.order_id();
    stamp.created = protoMsg.created();
    stamp.fulfilled = protoMsg.fulfilled();
    stamp.notified = protoMsg.notified();

    consumer.post(stamp);
    return true;
  }
};

} // namespace hft::serialization

#endif // HFT_COMMON_SERIALIZATION_PROTOMETADATASERIALIZER_HPP
