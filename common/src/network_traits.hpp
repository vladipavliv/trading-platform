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

#include "network/transport/shm/shm_transport.hpp"
#include "serialization/proto/proto_metadata_serializer.hpp"

namespace hft {

using StreamTransport = ShmTransport;
using DatagramTransport = ShmTransport;

#ifdef SERIALIZATION_SBE
using DomainSerializer = serialization::sbe::SbeDomainSerializer;
using Framer = DummyFramer<DomainSerializer>;
#else
using DomainSerializer = serialization::fbs::FbsDomainSerializer;
using Framer = FixedSizeFramer<DomainSerializer>;
#endif

using MetadataSerializer = serialization::proto::ProtoMetadataSerializer;

} // namespace hft

#endif // HFT_COMMON_NETWORKTRAITS_HPP