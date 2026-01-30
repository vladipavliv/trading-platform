/**
 * @author Vladimir Pavliv
 * @date 2026-01-14
 */

#ifndef HFT_SERVER_ORDERGATEWAY_HPP
#define HFT_SERVER_ORDERGATEWAY_HPP

#include "bus/bus_hub.hpp"
#include "config/server_config.hpp"
#include "containers/huge_array.hpp"
#include "domain/server_order_messages.hpp"
#include "id/slot_id_pool.hpp"
#include "internal_order.hpp"
#include "internal_order_status.hpp"
#include "logging.hpp"
#include "order_record.hpp"
#include "primitive_types.hpp"
#include "ptr_types.hpp"
#include "runner/lfq_runner.hpp"
#include "traits.hpp"
#include "utils/handler.hpp"

namespace hft::server {

/**
 * @brief Maintains order storage
 * strips down metadata of incoming orders producing internal events, supplies needed metadata to
 * outgoing order status messages, uses a separate thread with LfqRunner to manage outgoing traffic
 *
 * Two threads meet here and work with order record storage: network and gateway threads
 * On normal order flow, network thread picks up id from atomic pool, creates record,
 * and never touches it untill id is released by the gateway on order close
 * On cancel/modify flow however, network thread needs to access the record while its active
 * thread safety is managed by only creating records in the network thread, and never cleaning them
 * up explicitly, instead setting atomic state to Closed, and releasing record id
 * So network thread either sees the closed record and rejects cancel/modify order,
 * or picks up its released id, and reuses it, so cancel order for previous record would fail due to
 * incorrect generation.
 * This only needs atomic record state, other variables are never changed after creation, except for
 * BookOrderId, which is published once by the gateway thread,
 * and not accessed by the network thread untill state becomes Accepted
 */
class OrderGateway {
  using SelfT = OrderGateway;

public:
  explicit OrderGateway(Context &ctx)
      : ctx_{ctx},
        worker_{*this, ctx_.bus, ctx_.stopToken, "gateway", ctx.config.coreGateway, true} {
    ctx_.bus.subscribe(CRefHandler<ServerOrder>::bind<SelfT, &SelfT::post>(this));
  }

  ~OrderGateway() { LOG_DEBUG_SYSTEM("~OrderGateway"); }

  void start() {
    LOG_DEBUG_SYSTEM("OrderGateway start");
    worker_.run([this]() { ctx_.bus.post(ComponentReady{Component::Gateway}); });
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
    ctx_.bus.post(ServerOrderStatus{
        r.clientId, {r.externalOId, r.systemOId.raw(), s.fillQty, s.fillPrice, s.state}});

    switch (s.state) {
    case OrderState::Cancelled:
    case OrderState::Rejected:
    case OrderState::Full:
      closeRecord(r);
      break;
    default:
      // update book id for fast modify access
      LOG_DEBUG("Link systemOId {} with bookOId {}", s.id.raw(), s.bookOId.raw());
      r.bookOId = s.bookOId;
      r.setState(RecordState::Accepted);
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
      ctx_.bus.post(
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
    SystemOrderId sysOId{so.order.id};

    auto &o = so.order;
    auto &r = recordMap_[sysOId.index()];
    if (r.getState() != RecordState::Accepted || so.clientId != r.clientId ||
        o.id != r.systemOId.raw()) {
      LOG_ERROR_SYSTEM("Failed to cancel order: {}", toString(so));
      return;
    }
    ctx_.bus.post(
        InternalOrderEvent{{sysOId, r.bookOId, o.quantity, o.price}, nullptr, o.ticker, o.action});
  }

  void newOrder(CRef<ServerOrder> so) {
    LOG_DEBUG("Creating order record {}", toString(so));
    auto &o = so.order;
    auto systemOId = idPool_.acquire();
    if (!systemOId) {
      LOG_ERROR_SYSTEM("Server opened order limit exceeded, rejecting {}", toString(so));
      ctx_.bus.post(
          ServerOrderStatus{so.clientId, {o.id, 0, o.quantity, o.price, OrderState::Rejected}});
      return;
    }
    OrderRecord &r = recordMap_[systemOId.index()];
    r.externalOId = o.id;
    r.systemOId = systemOId;
    r.bookOId = BookOrderId{};
    r.clientId = so.clientId;
    r.ticker = o.ticker;
    r.setState(RecordState::New);

    ctx_.bus.post(InternalOrderEvent{
        {systemOId, BookOrderId{}, o.quantity, o.price}, nullptr, o.ticker, o.action});
  }

  void closeRecord(OrderRecord &r) {
    r.setState(RecordState::Closed);
    idPool_.release(r.systemOId);
  }

  inline bool isValid(CRef<ServerOrder> o) const noexcept { return o.order.price > 0; }

private:
  ALIGN_CL Context &ctx_;

  ALIGN_CL SlotIdPool<> idPool_;
  ALIGN_CL HugeArray<OrderRecord, SlotIdPool<>::CAPACITY> recordMap_;
  ALIGN_CL LfqRunner<InternalOrderStatus, OrderGateway, ServerBus> worker_;

  ALIGN_CL AtomicBool closed_{false};
};
} // namespace hft::server

#endif // HFT_SERVER_ORDERGATEWAY_HPP
