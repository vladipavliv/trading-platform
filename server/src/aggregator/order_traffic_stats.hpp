/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_ORDERTRAFFICSTATS_HPP
#define HFT_SERVER_ORDERTRAFFICSTATS_HPP

#include "boost_types.hpp"
#include "types.hpp"

namespace hft::server {

struct OrderTrafficStats {
  size_t ordersTotal{0};
  size_t ordersClosed{0};
  Timestamp timestamp{std::chrono::system_clock::now()};

  bool operator==(const OrderTrafficStats &other) const {
    return ordersTotal == other.ordersTotal && ordersClosed == other.ordersClosed;
  }
};

} // namespace hft::server

#endif // HFT_SERVER_ORDERTRAFFICSTATS_HPP