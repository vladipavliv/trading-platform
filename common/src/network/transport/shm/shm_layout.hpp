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
  std::atomic<bool> clientConnected{false};
  std::atomic<bool> serverReady{false};

  alignas(64) std::atomic<uint32_t> futex{0};

  ShmRingBuffer upstream;
  ShmRingBuffer downstream;
  ShmRingBuffer broadcast;
};

} // namespace hft

#endif // HFT_COMMON_SHMLAYOUT_HPP