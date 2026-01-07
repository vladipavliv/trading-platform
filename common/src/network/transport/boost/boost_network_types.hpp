/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_COMMON_BOOSTNETWORKTYPES_HPP
#define HFT_COMMON_BOOSTNETWORKTYPES_HPP

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/write.hpp>

#include "execution.hpp"

namespace hft {
using Tcp = boost::asio::ip::tcp;
using TcpSocket = boost::asio::ip::tcp::socket;
using TcpAcceptor = boost::asio::ip::tcp::acceptor;
using TcpEndpoint = boost::asio::ip::tcp::endpoint;
using TcpResolver = boost::asio::ip::tcp::resolver;

using Udp = boost::asio::ip::udp;
using UdpSocket = boost::asio::ip::udp::socket;
using UdpEndpoint = boost::asio::ip::udp::endpoint;
} // namespace hft

#endif // HFT_COMMON_BOOSTNETWORKTYPES_HPP