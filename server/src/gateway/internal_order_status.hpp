/**
 * @author Vladimir Pavliv
 * @date 2026-01-14
 */

#ifndef HFT_SERVER_INTERNALORDERSTATUS_HPP
#define HFT_SERVER_INTERNALORDERSTATUS_HPP

#include "domain_types.hpp"
#include "primitive_types.hpp"
#include "schema.hpp"
#include "ticker.hpp"
#include "utils/string_utils.hpp"

namespace hft::server {

struct InternalOrderStatus {
  SystemOrderId id;
  BookOrderId bookOId;
  Quantity fillQty;
  Price fillPrice;
  OrderState state;
};
} // namespace hft::server

namespace hft {
inline String toString(const server::InternalOrderStatus &event) {
  return std::format("InternalOrderStatus {} {} {} {} {}", event.id.raw(), event.bookOId.raw(),
                     event.fillQty, event.fillPrice, toString(event.state));
}
} // namespace hft

#endif // HFT_SERVER_INTERNALORDERSTATUS_HPP