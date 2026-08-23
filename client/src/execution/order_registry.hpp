/**
 * @author Vladimir Pavliv
 * @date 2026-08-23
 */

#ifndef HFT_CLIENT_ORDERREGISTRY_HPP
#define HFT_CLIENT_ORDERREGISTRY_HPP

#include "constants.hpp"
#include "containers/huge_array.hpp"
#include "containers/huge_queue.hpp"
#include "domain_types.hpp"
#include "id/slot_id_pool.hpp"
#include "primitive_types.hpp"

namespace hft::client {

using LocalOId = SlotIdPool<>::IdType;  // client order id
using SystemOId = SlotIdPool<>::IdType; // server order id

enum class RecordState : uint32_t { Empty, Pending, Active, Closed };

/**
 * @brief Tracks the orders on the client side
 */
struct OrderRecord {
  Order order;
  Timestamp created;
  SystemOId sysOId;
  // false sharing is possible here, but very rarely
  // engine thread writes new entries, and checks the timestamp and state of the head
  // network thread updates the state of entries upon receiving the status from the server
  // doesnt seem to be worth blowing the size of containers several times
  mutable RecordState state;

  bool isValid() const noexcept { return created != 0; }
  bool isClosed() const noexcept { return getState() == RecordState::Closed; }
  void setState(RecordState newState) const {
    std::atomic_ref stateRef{state};
    stateRef.store(newState, std::memory_order_release);
  }
  auto getState() const -> RecordState {
    std::atomic_ref stateRef{state};
    return stateRef.load(std::memory_order_acquire);
  }
};

/**
 * @brief Holds the order records, tracks the active orders queue
 */
struct OrderRegistry {
  bool allocate(Order &order, Timestamp now) {
    const auto id = idPool.acquire();
    if (!id) {
      LOG_ERROR_SYSTEM("Failed to acquire fresh id in OrderRegistry");
      return false;
    }
    const auto idx = id.index();
    order.id = id.raw();
    records[idx] = {order, now, SystemOId{}, RecordState::Pending};
    orderQueue.push(id);

    return true;
  }

  void deallocate(LocalOId id) {
    if (!id.isValid()) {
      LOG_ERROR_SYSTEM("Invalid LocalOId in OrderRegistry::deallocate");
      return;
    }
    idPool.release(id);
  }

  SlotIdPool<> idPool;
  HugeArray<OrderRecord, MAX_SYSTEM_ORDERS> records;
  HugeQueue<LocalOId, MAX_SYSTEM_ORDERS> orderQueue;

  ALIGN_CL AtomicUInt64 accepted{0};
  ALIGN_CL AtomicUInt64 fulfilled{0};
  ALIGN_CL AtomicUInt64 cancelled{0};
  ALIGN_CL AtomicUInt64 rejected{0};
};

} // namespace hft::client

#endif // HFT_CLIENT_ORDERREGISTRY_HPP
