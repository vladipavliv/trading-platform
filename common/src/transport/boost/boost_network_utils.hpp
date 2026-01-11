/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_BOOSTNETWORKUTILS_HPP
#define HFT_COMMON_BOOSTNETWORKUTILS_HPP

#include <boost/asio/bind_allocator.hpp>

#include "boost_network_types.hpp"
#include "io_result.hpp"
#include "logging.hpp"

namespace hft {
IoResult toIoResult(BoostErrorCode ec) noexcept {
  if (!ec) {
    return IoResult::Ok;
  }
  switch (ec.value()) {
  case boost::asio::error::operation_aborted:
  case boost::asio::error::eof:
    return IoResult::Closed;
  case boost::asio::error::would_block:
    return IoResult::WouldBlock;
  default:
    LOG_ERROR("{}", ec.message());
    return IoResult::Error;
  }
}
} // namespace hft

#endif // HFT_COMMON_BOOSTNETWORKUTILS_HPP