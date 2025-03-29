/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_NETWORK_TYPES_HPP
#define HFT_COMMON_NETWORK_TYPES_HPP

#include "boost_types.hpp"

namespace hft {

using Port = uint16_t;

namespace Ip = boost::asio::ip;
using Tcp = boost::asio::ip::tcp;
using TcpSocket = boost::asio::ip::tcp::socket;
using TcpEndpoint = boost::asio::ip::tcp::endpoint;
using TcpAcceptor = boost::asio::ip::tcp::acceptor;

using Udp = boost::asio::ip::udp;
using UdpSocket = boost::asio::ip::udp::socket;
using UdpEndpoint = boost::asio::ip::udp::endpoint;

enum class SocketType : uint8_t { Ingress = 0U, Egress = 1U, Broadcast = 2U };

using SocketCallback = std::function<void(const BoostError &, size_t)>;
using SocketStatusCallback = std::function<void(const BoostError &)>;

using MessageSize = uint16_t;

} // namespace hft

#endif // HFT_COMMON_NETWORK_TYPES_HPP