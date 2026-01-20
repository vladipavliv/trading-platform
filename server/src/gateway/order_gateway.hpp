/**
 * @author Vladimir Pavliv
 * @date 2026-01-14
 */

#ifndef HFT_SERVER_ORDERGATEWAY_HPP
#define HFT_SERVER_ORDERGATEWAY_HPP

#include <boost/unordered/unordered_flat_map.hpp>

#include "bus/bus_hub.hpp"
#include "config/server_config.hpp"
#include "domain/server_order_messages.hpp"
#include "id/slot_id_pool.hpp"
#include "internal_order.hpp"
#include "internal_order_status.hpp"
#include "logging.hpp"
#include "order_record.hpp"
#include "primitive_types.hpp"
#include "ptr_types.hpp"
#include "traits.hpp"
#include "utils/huge_array.hpp"
#include "utils/lfq_runner.hpp"

namespace hft::server {

/**
 * @brief Maintains order storage
 * strips down metadata of incoming orders producing internal events
 * supplies needed metadata to outgoing order status messages
 * maintans a thread with LfqRunner to manage outgoing traffic
 * network thread meets here with gateway thread,
 * sync is managed by lock-free handing over of order entry ids via spsc lifo id queue
 */
class OrderGateway {
  using OrderMapping = boost::unordered_flat_map<CompositeKey, SystemOrderId>;

public:
  explicit OrderGateway(ServerBus &bus)
      : bus_{bus}, worker_{*this, bus_, "gateway", ServerConfig::cfg().coreGateway, true} {
    bus_.subscribe<ServerOrder>(
        CRefHandler<ServerOrder>::template bind<OrderGateway, &OrderGateway::post>(this));
  }

  ~OrderGateway() { LOG_DEBUG_SYSTEM("~OrderGateway"); }

  void start() {
    LOG_DEBUG_SYSTEM("OrderGateway start");
    worker_.run();
  }

  void stop() {
    LOG_DEBUG_SYSTEM("OrderGateway stop");
    closed_.store(true, std::memory_order_release);
    worker_.stop();
  };

  void post(CRef<InternalOrderStatus> s) {
    LOG_DEBUG("{}", toString(s));
    if (closed_.load(std::memory_order_acquire)) {
      LOG_WARN_SYSTEM("OrderGateway is already stopped");
      return;
    }
    auto &r = recordMap_[s.id.index()];
    bus_.post(ServerOrderStatus{
        r.clientId, {r.externalOId, r.systemOId.raw(), s.fillQty, s.fillPrice, s.state}});

    switch (s.state) {
    case OrderState::Cancelled:
    case OrderState::Rejected:
    case OrderState::Full:
      cleanupOrder(s);
      break;
    default:
      // update book id for easier access
      LOG_DEBUG("Link systemOId {} with bookOId {}", s.id.raw(), s.bookOId.raw());
      r.bookOId = s.bookOId;
      break;
    }
  }

private:
  void post(CRef<ServerOrder> so) {
    LOG_DEBUG("{}", toString(so));
    if (closed_.load(std::memory_order_acquire)) {
      LOG_WARN_SYSTEM("OrderGateway is already stopped");
      return;
    }
    auto &o = so.order;
    if (!isValid(so)) {
      LOG_ERROR_SYSTEM("Invalid order {}", toString(so));
      bus_.post(
          ServerOrderStatus{so.clientId, {o.id, 0, o.quantity, o.price, OrderState::Rejected}});
      bus_.post(
          ServerOrderStatus{so.clientId, {o.id, 0, o.quantity, o.price, OrderState::Rejected}});
      return;
    }
    switch (o.action) {
    case OrderAction::Dummy:
      LOG_DEBUG("Dummy order received");
      break;
    case OrderAction::Cancel:
      cancelOrder(so);
      break;
    case OrderAction::Modify:
      LOG_DEBUG("Modify request received");
      // TODO(self)
      break;
    default:
      newOrder(so);
      break;
    }
  }

  void cancelOrder(CRef<ServerOrder> so) {
    LOG_DEBUG("Cancel order: {}", toString(so));
    // initially client sends his internal oid, when cancel/modify - system oid sent in status msg
    SystemOrderId sysOId{so.order.id};

    auto &o = so.order;
    auto &r = recordMap_[sysOId.index()];
    if (so.clientId != r.clientId || o.id != r.systemOId.raw()) {
      LOG_ERROR_SYSTEM("Failed to cancel order: {}", toString(so));
      return;
    }
    bus_.post(
        InternalOrderEvent{{sysOId, r.bookOId, o.quantity, o.price}, nullptr, o.ticker, o.action});
  }

  void newOrder(CRef<ServerOrder> so) {
    LOG_DEBUG("Creating order record {}", toString(so));
    auto &o = so.order;
    auto systemOId = idPool_.acquire();
    if (!systemOId) {
      LOG_ERROR_SYSTEM("Server opened order limit exceeded, rejecting {}", toString(so));
      bus_.post(
          ServerOrderStatus{so.clientId, {o.id, 0, o.quantity, o.price, OrderState::Rejected}});
      return;
    }
    recordMap_[systemOId.index()] = {o.id, systemOId, BookOrderId{}, so.clientId, o.ticker};
    bus_.post(InternalOrderEvent{
        {systemOId, BookOrderId{}, o.quantity, o.price}, nullptr, o.ticker, o.action});
  }

  void cleanupOrder(CRef<InternalOrderStatus> ios) {
    LOG_DEBUG("{}", toString(ios));
    auto &r = recordMap_[ios.id.index()];
    r = {};
    idPool_.release(ios.id);
  }

  inline bool isValid(CRef<ServerOrder> o) const noexcept { return o.order.price > 0; }

private:
  ALIGN_CL ServerBus &bus_;

  ALIGN_CL SlotIdPool<> idPool_;
  ALIGN_CL HugeArray<OrderRecord, SlotIdPool<>::CAPACITY> recordMap_;

  ALIGN_CL LfqRunner<InternalOrderStatus, OrderGateway, ServerBus> worker_;

  ALIGN_CL AtomicBool closed_{false};
};
} // namespace hft::server

#endif // HFT_SERVER_ORDERGATEWAY_HPP
