/**
 * @author Vladimir Pavliv
 * @date 2025-01-06
 */

#ifndef HFT_COMMON_NETWORKTRAITS_HPP
#define HFT_COMMON_NETWORKTRAITS_HPP

#ifdef SERIALIZATION_SBE
#include "serialization/sbe/sbe_domain_serializer.hpp"
#include "transport/framing/dummy_framer.hpp"
#else
#include "serialization/fbs/fbs_domain_serializer.hpp"
#include "transport/framing/fixed_size_framer.hpp"
#endif

namespace hft {
#ifdef SERIALIZATION_SBE
using DomainSerializer = serialization::sbe::SbeDomainSerializer;
using Framer = DummyFramer<DomainSerializer>;
#else
using DomainSerializer = serialization::fbs::FbsDomainSerializer;
using Framer = FixedSizeFramer<DomainSerializer>;
#endif

// TODO(self): remove this
} // namespace hft

#endif // HFT_COMMON_NETWORKTRAITS_HPP