/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_SHMLAYOUT_HPP
#define HFT_COMMON_SHMLAYOUT_HPP

#include <atomic>

#include "network/async_transport.hpp"
#include "shm_ring_buffer.hpp"

namespace hft {

/**
 * @brief Shared memory layout
 */
struct ShmLayout {
  alignas(64) std::atomic<uint32_t> upstreamFtx{0};
  alignas(64) std::atomic<uint32_t> downstreamFtx{0};

  alignas(64) std::atomic<bool> upstreamWaiting{false};
  alignas(64) std::atomic<bool> downstreamWaiting{false};

  alignas(64) ShmRingBuffer upstream;
  alignas(64) ShmRingBuffer downstream;
  alignas(64) ShmRingBuffer broadcast;
};

} // namespace hft

#endif // HFT_COMMON_SHMLAYOUT_HPP