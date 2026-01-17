/**
 * @author Vladimir Pavliv
 * @date 2026-01-16
 */

#ifndef HFT_COMMON_SCHEMA_HPP
#define HFT_COMMON_SCHEMA_HPP

#include "constants.hpp"
#include "id/slot_id.hpp"
#include "primitive_types.hpp"

namespace hft::server {

using SystemOrderId = SlotId<MAX_SYSTEM_ORDERS>;
using BookOrderId = SlotId<MAX_BOOK_ORDERS>;

} // namespace hft::server

#endif // HFT_SERVER_SCHEMA_HPP