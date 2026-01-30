/**
 * @author Vladimir Pavliv
 * @date 2026-01-14
 */

#ifndef HFT_SERVER_ORDERRECORD_HPP
#define HFT_SERVER_ORDERRECORD_HPP

#include "domain_types.hpp"
#include "primitive_types.hpp"
#include "schema.hpp"

namespace hft::server {
enum class RecordState : uint32_t { New, Accepted, Closed };

/**
 * @brief RecordState is made raw with atomic_ref access to keep record trivially constructible
 */
struct OrderRecord {
  // Immutable after create
  OrderId externalOId;
  SystemOrderId systemOId;
  BookOrderId bookOId;
  Ticker ticker;

  // published once
  ClientId clientId;

  // lifecycle
  RecordState state;

  inline RecordState getState() const {
    std::atomic_ref<const RecordState> aState(state);
    return aState.load(std::memory_order_acquire);
  }

  inline void setState(RecordState newState) {
    std::atomic_ref<RecordState> aState(state);
    aState.store(newState, std::memory_order_release);
  }
};
} // namespace hft::server

#endif // HFT_SERVER_ORDERRECORD_HPP