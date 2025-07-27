/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_SERIALIZATION_PROTOCONVERTER_HPP
#define HFT_COMMON_SERIALIZATION_PROTOCONVERTER_HPP

#include "domain_types.hpp"
#include "gen/proto/metadata_messages.pb.h"
#include "logging.hpp"
#include "metadata_types.hpp"
#include "types.hpp"

namespace hft::serialization {

MetadataSource convert(gen::proto::metadata::Source source) {
  switch (source) {
  case gen::proto::metadata::Source::CLIENT:
    return MetadataSource::Client;
  case gen::proto::metadata::Source::SERVER:
    return MetadataSource::Server;
  default:
    return MetadataSource::Unknown;
  }
}

gen::proto::metadata::Source convert(MetadataSource source) {
  switch (source) {
  case MetadataSource::Client:
    return gen::proto::metadata::Source::CLIENT;
  case MetadataSource::Server:
    return gen::proto::metadata::Source::SERVER;
  default:
    return gen::proto::metadata::Source::UNKNOWN;
  }
}

} // namespace hft::serialization

#endif // HFT_COMMON_SERIALIZATION_PROTOCONVERTER_HPP