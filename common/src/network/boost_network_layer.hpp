/**
 * @author Vladimir Pavliv
 * @date 2025-05-01
 */

#ifndef HFT_COMMON_BOOST_NETWORKLAYER_HPP
#define HFT_COMMON_BOOST_NETWORKLAYER_HPP

#include "boost_types.hpp"
#include "types.hpp"

namespace hft {

namespace Ip = boost::asio::ip;

using Tcp = boost::asio::ip::tcp;
using TcpSocket = boost::asio::ip::tcp::socket;
using TcpEndpoint = boost::asio::ip::tcp::endpoint;
using TcpAcceptor = boost::asio::ip::tcp::acceptor;

using Udp = boost::asio::ip::udp;
using UdpSocket = boost::asio::ip::udp::socket;
using UdpEndpoint = boost::asio::ip::udp::endpoint;

class NetworkFactory {
public:
  /**
   * Tcp
   */
  static TcpEndpoint createTcpEndpoint(Port port) { return TcpEndpoint{Tcp::v4(), port}; }
  static TcpEndpoint createTcpEndpoint(CRef<String> url, Port port) {
    return TcpEndpoint{Ip::make_address(url), port};
  }
  static TcpSocket createTcpSocket(IoCtx &ctx) { return TcpSocket(ctx); }
  static TcpAcceptor createTcpAcceptor(IoCtx &ctx) { return TcpAcceptor(ctx); }

  /**
   * Udp
   */
  static UdpEndpoint createUdpEndpoint(bool broadcast, Port port) {
    return broadcast ? UdpEndpoint{Ip::address_v4::broadcast(), port}
                     : UdpEndpoint(Udp::v4(), port);
  }
  static UdpSocket createUdpSocket(IoCtx &ctx, bool broadcast = true, Port port = 0) {
    UdpSocket socket(ctx, Udp::v4());
    socket.set_option(boost::asio::socket_base::reuse_address{true});
    if (broadcast) {
      socket.set_option(boost::asio::socket_base::broadcast(true));
    } else {
      socket.bind(UdpEndpoint(Udp::v4(), port));
    }
    return socket;
  }
};
} // namespace hft

#endif // HFT_COMMON_BOOST_NETWORKLAYER_HPP
