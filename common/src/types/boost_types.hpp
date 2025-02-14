/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_BOOST_TYPES_HPP
#define HFT_COMMON_BOOST_TYPES_HPP

#include <boost/asio.hpp>
#include <boost/fiber/all.hpp>

namespace hft {

namespace Ip = boost::asio::ip;
using IoContext = boost::asio::io_context;
using ContextGuard = boost::asio::io_context::work;

using BoostError = boost::system::error_code;
using BoostErrorRef = const BoostError &;

using Fiber = boost::fibers::fiber;

using Seconds = boost::asio::chrono::seconds;
using MilliSeconds = boost::asio::chrono::milliseconds;

} // namespace hft

#endif // HFT_COMMON_BOOST_TYPES_HPP
