/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_BOOST_TYPES_HPP
#define HFT_COMMON_BOOST_TYPES_HPP

#include <boost/asio.hpp>

namespace hft {

namespace Ip = boost::asio::ip;

using IoContext = boost::asio::io_context;
using UPtrIoContext = std::unique_ptr<boost::asio::io_context>;
using ContextGuard = boost::asio::executor_work_guard<IoContext::executor_type>;
using UPtrContextGuard = std::unique_ptr<ContextGuard>;

using BoostError = boost::system::error_code;
using BoostErrorRef = const BoostError &;

using SteadyTimer = boost::asio::steady_timer;
using Seconds = boost::asio::chrono::seconds;
using Milliseconds = boost::asio::chrono::milliseconds;
using Microseconds = boost::asio::chrono::microseconds;
using Timestamp = std::chrono::time_point<std::chrono::system_clock>;

} // namespace hft

#endif // HFT_COMMON_BOOST_TYPES_HPP
