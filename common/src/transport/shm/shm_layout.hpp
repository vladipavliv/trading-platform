/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_SHMLAYOUT_HPP
#define HFT_COMMON_SHMLAYOUT_HPP

#include <atomic>

#include "containers/sequenced_spsc.hpp"
#include "primitive_types.hpp"
#include "transport/async_transport.hpp"

namespace hft {

/**
 * @brief Shared memory layout
 */
struct ShmLayout {
  alignas(64) AtomicUInt32 upstreamFtx{0};
  alignas(64) AtomicUInt32 downstreamFtx{0};

  alignas(64) AtomicBool upstreamWaiting{false};
  alignas(64) AtomicBool downstreamWaiting{false};

  alignas(64) SequencedSPSC upstream;
  alignas(64) SequencedSPSC downstream;
  alignas(64) SequencedSPSC broadcast;
  alignas(64) SequencedSPSC telemetry;
};

} // namespace hft

#endif // HFT_COMMON_SHMLAYOUT_HPP