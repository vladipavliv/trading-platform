/**
 * @author Vladimir Pavliv
 * @date 2026-01-16
 */

#ifndef HFT_SERVER_SCHEMA_HPP
#define HFT_SERVER_SCHEMA_HPP

#include "id/slot_id.hpp"
#include "primitive_types.hpp"

namespace hft::server {

static constexpr uint32_t MAX_SYSTEM_ORDERS = 16'777'216;
static constexpr uint32_t MAX_BOOK_ORDERS = 131'072;
static constexpr uint32_t MAX_TICKS = 100'000;

/**
 * @brief Internal server-side id
 */
using SystemOrderId = SlotId<MAX_SYSTEM_ORDERS>;
/**
 * @brief Local to OrderBook id
 */
using BookOrderId = SlotId<MAX_BOOK_ORDERS>;
} // namespace hft::server

#endif // HFT_SERVER_SCHEMA_HPP