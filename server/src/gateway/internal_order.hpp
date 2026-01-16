/**
 * @author Vladimir Pavliv
 * @date 2026-01-14
 */

#ifndef HFT_SERVER_INTERNALORDER_HPP
#define HFT_SERVER_INTERNALORDER_HPP

#include "domain_types.hpp"
#include "id/slot_id.hpp"
#include "primitive_types.hpp"
#include "schema.hpp"
#include "ticker.hpp"
#include "utils/string_utils.hpp"

namespace hft::server {

struct TickerData;

struct InternalOrder {
  SystemOrderId id;
  BookOrderId bookOId;
  Quantity quantity;
  Price price;

  inline void partialFill(Quantity fill) { quantity = quantity < fill ? 0 : quantity - fill; }
  inline bool isFilled() const { return quantity == 0; }
};

/**
 * @brief event to route IntOrder to OrderBook and store only the needed data internally
 */
struct InternalOrderEvent {
  InternalOrder order;
  mutable const TickerData *data{nullptr};
  Ticker ticker;
  OrderAction action;
};

} // namespace hft::server

namespace hft {
inline String toString(const server::InternalOrder &event) {
  return std::format("InternalOrder {} {} {} {}", event.id.raw(), event.bookOId.raw(),
                     event.quantity, event.price);
}
inline String toString(const server::InternalOrderEvent &e) {
  return std::format("InternalOrderEvent {} {} {}", toString(e.order), toString(e.action),
                     toString(e.ticker));
}
} // namespace hft

#endif // HFT_SERVER_INTERNALORDER_HPP