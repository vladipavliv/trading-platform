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
 * @brief
 */
class OrderGateway {
  using OrderMapping = boost::unordered_flat_map<CompositeKey, InternalOrderId>;

public:
  explicit OrderGateway(ServerBus &bus)
      : bus_{bus}, worker_{*this, bus_, ServerConfig::cfg.coreGateway, true} {
    bus_.subscribe<ServerOrder>(
        CRefHandler<ServerOrder>::template bind<OrderGateway, &OrderGateway::post>(this));
  }

  void start() {
    LOG_DEBUG_SYSTEM("OrderGateway start");
    worker_.run();
  }

  void stop() {
    LOG_DEBUG_SYSTEM("OrderGateway stop");
    worker_.stop();
  };

  void post(CRef<InternalOrderStatus> s) {
    LOG_DEBUG("{}", toString(s));
    auto &r = recordMap_[s.id.index()];
    bus_.post(
        ServerOrderStatus{r.clientId, {r.id.raw(), r.created, s.fillQty, s.fillPrice, s.state}});

    switch (s.state) {
    case OrderState::Cancelled:
    case OrderState::Rejected:
    case OrderState::Full:
      cleanupOrder(s);
      break;
    default:
      break;
    }
  }

private:
  void post(CRef<ServerOrder> so) {
    LOG_DEBUG("{}", toString(so));
    if (!isValid(so)) {
      LOG_ERROR("Invalid order {}", toString(so));
      bus_.post(ServerOrderStatus{so.clientId,
                                  {so.order.id, so.order.created, 0, 0, OrderState::Rejected}});
      return;
    }
    switch (so.order.action) {
    case OrderAction::Dummy:
      break;
    case OrderAction::Cancel:
      cancelOrder(so);
      break;
    case OrderAction::Modify:
      // TODO(self)
      break;
    default:
      newOrder(so);
      break;
    }
  }

  void cancelOrder(CRef<ServerOrder> so) {
    LOG_DEBUG("{}", toString(so));
    InternalOrderId id{so.order.id};

    auto &o = so.order;
    auto &r = recordMap_[id.index()];
    if (so.clientId != r.clientId || o.id != r.id.raw()) {
      LOG_ERROR_SYSTEM("Failed to cancel order: {}", toString(so));
      return;
    }
    bus_.post(InternalOrderEvent{{r.id, o.quantity, o.price}, nullptr, o.ticker, o.action});
  }

  void newOrder(CRef<ServerOrder> so) {
    LOG_DEBUG("{}", toString(so));
    auto &o = so.order;
    auto id = idPool_.acquire();
    if (!id) {
      bus_.post(ServerOrderStatus{so.clientId, {o.id, o.created, 0, 0, OrderState::Rejected}});
      return;
    }
    recordMap_[id.index()] = {o.created, id, so.clientId};
    bus_.post(InternalOrderEvent{{id, o.quantity, o.price}, nullptr, o.ticker, o.action});
    // bus_.post(ServerOrderStatus{so.clientId, {id.raw(), o.created, 0, 0, OrderState::Accepted}});
  }

  void cleanupOrder(CRef<InternalOrderStatus> ios) {
    LOG_DEBUG("{}", toString(ios));
    auto &r = recordMap_[ios.id.index()];
    r = {};
    idPool_.release(ios.id);
  }

  inline bool isValid(CRef<ServerOrder> o) const noexcept {
    return o.order.quantity > 0 && o.order.price > 0;
  }

private:
  ServerBus &bus_;

  ALIGN_CL SlotIdPool<> idPool_;
  ALIGN_CL HugeArray<OrderRecord, SlotIdPool<>::Capacity> recordMap_;

  ALIGN_CL LfqRunner<InternalOrderStatus, OrderGateway, ServerBus> worker_;
};
} // namespace hft::server

#endif // HFT_SERVER_ORDERGATEWAY_HPP