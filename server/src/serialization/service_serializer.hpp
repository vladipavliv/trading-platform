/**
 * @author Vladimir Pavliv
 * @date 2025-04-08
 */

#ifndef HFT_SERVER_SERIALIZATION_SERVICESERIALIZER_HPP
#define HFT_SERVER_SERIALIZATION_SERVICESERIALIZER_HPP

#include "converter.hpp"
#include "gen/servercmd_generated.h"
#include "metadata_types.hpp"
#include "template_types.hpp"

namespace hft::server::serialization::fbs {

class ServiceSerializer {
public:
  using BufferType = flatbuffers::DetachedBuffer;
  using RootMessage = hft::serialization::gen::fbs::ServerCommand;

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
    consumer.post(ServerCommand{convert(msg->command())});
    return true;
  }

  static BufferType serialize(CRef<ServerCommand> cmd) {
    LOG_DEBUG("Serializing {}", utils::toString(cmd));
    using namespace hft::serialization::gen::fbs;
    flatbuffers::FlatBufferBuilder builder;
    const auto msg = CreateServerCommand(builder, convert(cmd));
    builder.Finish(msg);
    return builder.Release();
  }
};

} // namespace hft::server::serialization::fbs

#endif // HFT_SERVER_SERIALIZATION_SERVICESERIALIZER_HPP
