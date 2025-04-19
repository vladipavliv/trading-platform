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

namespace Ip = boost::asio::ip;

using IoCtx = boost::asio::io_context;
using UPtrIoCtx = std::unique_ptr<boost::asio::io_context>;
using ContextGuard = boost::asio::executor_work_guard<IoCtx::executor_type>;
using UPtrContextGuard = std::unique_ptr<ContextGuard>;

using BoostError = boost::system::error_code; // TODO

using SteadyTimer = boost::asio::steady_timer;
using Seconds = boost::asio::chrono::seconds;
using Milliseconds = boost::asio::chrono::milliseconds;
using Microseconds = boost::asio::chrono::microseconds;

using ConstBuffer = boost::asio::const_buffer;
using MutableBuffer = boost::asio::mutable_buffer;

using MakeGuard = decltype(boost::asio::make_work_guard(std::declval<boost::asio::io_context &>()));

using LittleEndianByte = boost::endian::little_uint8_at;
using LittleEndianUInt16 = boost::endian::little_uint16_at;

using Port = uint16_t;

namespace Ip = boost::asio::ip;
using Tcp = boost::asio::ip::tcp;
using TcpSocket = boost::asio::ip::tcp::socket;
using TcpEndpoint = boost::asio::ip::tcp::endpoint;
using TcpAcceptor = boost::asio::ip::tcp::acceptor;

using Udp = boost::asio::ip::udp;
using UdpSocket = boost::asio::ip::udp::socket;
using UdpEndpoint = boost::asio::ip::udp::endpoint;

using SocketCallback = std::function<void(const BoostError &, size_t)>;
using SocketStatusCallback = std::function<void(const BoostError &)>;

using MessageSize = uint16_t;

} // namespace hft

#endif // HFT_COMMON_BOOST_TYPES_HPP
