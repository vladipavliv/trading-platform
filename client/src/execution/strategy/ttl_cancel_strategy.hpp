/**
 * @author Vladimir Pavliv
 * @date 2026-08-23
 */

#ifndef HFT_COMMON_TTLCANCELSTRATEGY_HPP
#define HFT_COMMON_TTLCANCELSTRATEGY_HPP

#include "config/client_config.hpp"
#include "execution/market_data.hpp"
#include "execution/order_registry.hpp"
#include "traits.hpp"

namespace hft::client {
class TtlCancelStrategy {
public:
  TtlCancelStrategy(Context &ctx, OrderRegistry &registry) : ctx_{ctx}, registry_{registry} {}

  void execute() {
    auto &queue = registry_.orderQueue;
    while (!queue.empty()) {
      auto &sid = queue.front();
      const auto idx = sid.index();
      auto &record = registry_.records[idx];

      // Check for id mismatch, it could have already been released and reused
      if (record.order.id != sid.raw()) {
        queue.pop();
        continue;
      }

      const auto state = record.getState();
      if (state == RecordState::Closed || state == RecordState::Empty) {
        queue.pop();
        continue;
      }
      if (state == RecordState::Pending) {
        break;
      }

      const auto now = utils::getCycles();
      auto &o = record.order;
      if (now - record.created >= ctx_.config.orderTtlCycles) {
        record.created = now;
        Order cancel{record.sysOId.raw(), o.ticker, o.quantity, o.price, OrderAction::Cancel};
        ctx_.bus.marketBus.post(cancel);
        queue.pop();
      }

      break;
    }
  }

private:
  Context &ctx_;
  OrderRegistry &registry_;
};
} // namespace hft::client

#endif // HFT_COMMON_TTLCANCELSTRATEGY_HPP
