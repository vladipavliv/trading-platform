/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_SHMLAYOUT_HPP
#define HFT_COMMON_SHMLAYOUT_HPP

#include <atomic>

#include "primitive_types.hpp"
#include "shm_queue.hpp"
#include "transport/async_transport.hpp"

namespace hft {

/**
 * @brief Shared memory layout
 */
struct ShmLayout {
  alignas(64) ShmQueue upstream;
  alignas(64) ShmQueue downstream;
  alignas(64) ShmQueue broadcast;
  alignas(64) ShmQueue telemetry;
};

} // namespace hft

#endif // HFT_COMMON_SHMLAYOUT_HPP