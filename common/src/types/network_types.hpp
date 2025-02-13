/**
 * @file network_types.hpp
 * @brief Network type aliases
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_NETWORK_TYPES_HPP
#define HFT_COMMON_NETWORK_TYPES_HPP

#include <boost/asio.hpp>

namespace hft {

using Port = uint16_t;
using SteadyTimer = boost::asio::steady_timer;

using Tcp = boost::asio::ip::tcp;
using TcpSocket = boost::asio::ip::tcp::socket;
using TcpEndpoint = boost::asio::ip::tcp::endpoint;
using TcpAcceptor = boost::asio::ip::tcp::acceptor;

using Udp = boost::asio::ip::udp;
using UdpSocket = boost::asio::ip::udp::socket;
using UdpEndpoint = boost::asio::ip::udp::endpoint;

using IoContext = boost::asio::io_context;
using ContextGuard = boost::asio::io_context::work;

using BoostError = boost::system::error_code;
using BoostErrorRef = const BoostError &;

using MessageSize = uint16_t; // smol

using Seconds = boost::asio::chrono::seconds;
using MilliSeconds = boost::asio::chrono::milliseconds;

} // namespace hft

#endif // HFT_COMMON_NETWORK_TYPES_HPP