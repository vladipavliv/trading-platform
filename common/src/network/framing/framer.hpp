/**
 * @author Vladimir Pavliv
 * @date 2025-08-12
 */

#ifndef HFT_COMMON_FRAMER_HPP
#define HFT_COMMON_FRAMER_HPP

#include "dummy_framer.hpp"
#include "fixed_size_framer.hpp"

namespace hft {

#ifdef SERIALIZATION_SBE
using DefaultFramer = DummyFramer<>;
#else
using DefaultFramer = FixedSizeFramer<>;
#endif
} // namespace hft

#endif // HFT_COMMON_FRAMER_HPP