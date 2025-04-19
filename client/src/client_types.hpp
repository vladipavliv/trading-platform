/**
 * @author Vladimir Pavliv
 * @date 2025-04-18
 */

#ifndef HFT_CLIENT_CLIENTTYPES_HPP
#define HFT_CLIENT_CLIENTTYPES_HPP

#include "bus/bus.hpp"
#include "domain_types.hpp"

namespace hft::client {

using Bus = BusHolder<Order, OrderStatus, TickerPrice>;

}

#endif // HFT_CLIENT_CLIENTTYPES_HPP