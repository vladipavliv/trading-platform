/**
 * @author Vladimir Pavliv
 * @date 2026-01-07
 */

#ifndef HFT_COMMON_EXECUTION_HPP
#define HFT_COMMON_EXECUTION_HPP

#include <boost/asio/buffer.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/system/error_code.hpp>

#include "functional_types.hpp"
#include "primitive_types.hpp"

namespace hft {
using IoCtx = boost::asio::io_context;
using IoCtxGuard = boost::asio::executor_work_guard<IoCtx::executor_type>;
using Strand = boost::asio::io_context::strand;

using MakeGuard = decltype(boost::asio::make_work_guard(std::declval<boost::asio::io_context &>()));

using SteadyTimer = boost::asio::steady_timer;
using BoostErrorCode = boost::system::error_code;

using ConstBuffer = boost::asio::const_buffer;
using MutableBuffer = boost::asio::mutable_buffer;

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;

inline const auto ERR_ABORTED = boost::asio::error::operation_aborted;
inline const auto ERR_EOF = boost::asio::error::eof;
} // namespace hft

#endif