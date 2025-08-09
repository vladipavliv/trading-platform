/**
 * @author Vladimir Pavliv
 * @date 2025-08-09
 */

#ifndef HFT_COMMON_SERIALIZATION_SERIALIZER_HPP
#define HFT_COMMON_SERIALIZATION_SERIALIZER_HPP

#ifdef SERIALIZATION_SBE
#include "sbe/sbe_domain_serializer.hpp"
#else
#include "fbs/fbs_domain_serializer.hpp"
#endif
#include "proto/proto_metadata_serializer.hpp"

namespace hft::serialization {

#ifdef SERIALIZATION_SBE
using DomainSerializer = sbe::SbeDomainSerializer;
#else
using DomainSerializer = fbs::FbsDomainSerializer;
#endif
using MetadataSerializer = proto::ProtoMetadataSerializer;

} // namespace hft::serialization

#endif // HFT_COMMON_SERIALIZATION_SERIALIZER_HPP