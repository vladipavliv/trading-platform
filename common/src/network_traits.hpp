/**
 * @author Vladimir Pavliv
 * @date 2025-01-06
 */

#ifndef HFT_COMMON_NETWORKTRAITS_HPP
#define HFT_COMMON_NETWORKTRAITS_HPP

#ifdef SERIALIZATION_SBE
#include "network/framing/dummy_framer.hpp"
#include "serialization/sbe/sbe_domain_serializer.hpp"
#else
#include "network/framing/fixed_size_framer.hpp"
#include "serialization/fbs/fbs_domain_serializer.hpp"
#endif

#ifdef TELEMETRY_ENABLED
#include "serialization/proto/proto_metadata_serializer.hpp"
#endif

namespace hft {
#ifdef SERIALIZATION_SBE
using DomainSerializer = serialization::sbe::SbeDomainSerializer;
using Framer = DummyFramer<DomainSerializer>;
#else
using DomainSerializer = serialization::fbs::FbsDomainSerializer;
using Framer = FixedSizeFramer<DomainSerializer>;
#endif

#ifdef TELEMETRY_ENABLED
using MetadataSerializer = serialization::proto::ProtoMetadataSerializer;
#endif

} // namespace hft

#endif // HFT_COMMON_NETWORKTRAITS_HPP