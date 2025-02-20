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
  size_t currentOrders{0};
  size_t processedOrders{0};
  Timestamp timestamp{std::chrono::system_clock::now()};
};

} // namespace hft::server

#endif // HFT_SERVER_ORDERTRAFFICSTATS_HPP