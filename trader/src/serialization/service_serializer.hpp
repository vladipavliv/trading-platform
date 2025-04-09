/**
 * @author Vladimir Pavliv
 * @date 2025-04-08
 */

#ifndef HFT_TRADER_SERIALIZATION_SERVICESERIALIZER_HPP
#define HFT_TRADER_SERIALIZATION_SERVICESERIALIZER_HPP

#include "converter.hpp"
#include "gen/tradercmd_generated.h"
#include "metadata_types.hpp"
#include "template_types.hpp"
#include "trader_command.hpp"

namespace hft::trader::serialization::fbs {

class ServiceSerializer {
public:
  using BufferType = flatbuffers::DetachedBuffer;
  using RootMessage = hft::serialization::gen::fbs::TraderCommand;

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
    consumer.post(TraderCommand{convert(msg->command())});
    return true;
  }

  static BufferType serialize(CRef<TraderCommand> cmd) {
    LOG_DEBUG("Serializing {}", utils::toString(cmd));
    using namespace hft::serialization::gen::fbs;
    flatbuffers::FlatBufferBuilder builder;
    const auto msg = CreateTraderCommand(builder, convert(cmd));
    builder.Finish(msg);
    return builder.Release();
  }
};

} // namespace hft::trader::serialization::fbs

#endif // HFT_TRADER_SERIALIZATION_SERVICESERIALIZER_HPP
