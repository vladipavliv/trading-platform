/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_SHMLAYOUT_HPP
#define HFT_COMMON_SHMLAYOUT_HPP

#include <atomic>

#include "network/async_transport.hpp"
#include "shm_ring_buffer.hpp"
#include "sloth_buffer.hpp"

namespace hft {

/**
 * @brief Shared memory layout
 */
struct ShmLayout {
  alignas(64) std::atomic<uint32_t> upstreamFtx{0};
  alignas(64) std::atomic<uint32_t> downstreamFtx{0};

  alignas(64) std::atomic<bool> upstreamWaiting{false};
  alignas(64) std::atomic<bool> downstreamWaiting{false};

  alignas(64) SlothBuffer upstream;
  alignas(64) SlothBuffer downstream;
  alignas(64) SlothBuffer broadcast;
};

} // namespace hft

#endif // HFT_COMMON_SHMLAYOUT_HPP