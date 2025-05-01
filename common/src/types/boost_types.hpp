/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_BOOST_TYPES_HPP
#define HFT_COMMON_BOOST_TYPES_HPP

#include <boost/asio.hpp>
#include <boost/endian/arithmetic.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/uuid/uuid.hpp>

namespace hft {

using IoCtx = boost::asio::io_context;
using ContextGuard = boost::asio::executor_work_guard<IoCtx::executor_type>;
using MakeGuard = decltype(boost::asio::make_work_guard(std::declval<boost::asio::io_context &>()));

using BoostErrorCode = boost::system::error_code;

using SteadyTimer = boost::asio::steady_timer;

using ConstBuffer = boost::asio::const_buffer;
using MutableBuffer = boost::asio::mutable_buffer;

using LittleEndianByte = boost::endian::little_uint8_at;
using LittleEndianUInt16 = boost::endian::little_uint16_at;

} // namespace hft

#endif // HFT_COMMON_BOOST_TYPES_HPP
