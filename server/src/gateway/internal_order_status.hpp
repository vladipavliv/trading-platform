/**
 * @author Vladimir Pavliv
 * @date 2026-01-14
 */

#ifndef HFT_SERVER_INTERNALORDERSTATUS_HPP
#define HFT_SERVER_INTERNALORDERSTATUS_HPP

#include "domain_types.hpp"
#include "internal_id.hpp"
#include "primitive_types.hpp"
#include "ticker.hpp"
#include "utils/string_utils.hpp"

namespace hft::server {

struct InternalOrderStatus {
  InternalOrderId id;
  Quantity fillQty;
  Price fillPrice;
  OrderState state;
};
} // namespace hft::server

namespace hft {
inline String toString(const server::InternalOrderStatus &event) {
  return std::format("InternalOrderStatus {} {} {} {}", event.id.raw(), toString(event.state),
                     event.fillPrice, event.fillQty);
}
} // namespace hft

#endif // HFT_SERVER_INTERNALORDERSTATUS_HPP