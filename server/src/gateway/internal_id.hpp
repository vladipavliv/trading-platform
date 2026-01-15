/**
 * @author Vladimir Pavliv
 * @date 2026-01-14
 */

#ifndef HFT_SERVER_INTERNALID_HPP
#define HFT_SERVER_INTERNALID_HPP

#include "id/slot_id.hpp"

namespace hft::server {
using InternalOrderId = SlotId<>;
}

#endif // HFT_SERVER_INTERNALID_HPP