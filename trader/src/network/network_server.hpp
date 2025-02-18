/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_NETWORK_SERVER_HPP
#define HFT_COMMON_NETWORK_SERVER_HPP

#include "boost_types.hpp"
#include "network_types.hpp"
#include "trader_types.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft::trader {

class NetworkServer {
public:
  using OrderSocket = TraderSocket<TcpSocket, OrderStatus>;
  using PriceSocket = TraderSocket<UdpSocket, TickerPrice>;

  NetworkServer(TraderSink &sink)
      : mSink{sink},
        mOrderSocket{sink, TcpSocket{sink.ctx()},
                     TcpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portTcpIn}},
        mStatusSocket{sink, TcpSocket{sink.ctx()},
                      TcpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portTcpOut}},
        mPriceSocket{sink, createUdpSocket(sink.ctx()),
                     UdpEndpoint(Udp::v4(), Config::cfg.portUdp)} {
    mSink.ioSink.setHandler<Order>([this](const Order &order) { mOrderSocket.asyncWrite(order); });
  }

  void start() {
    mOrderSocket.asyncConnect();
    mStatusSocket.asyncConnect();
    mPriceSocket.asyncConnect();
  }

private:
  UdpSocket createUdpSocket(IoContext &ctx) {
    UdpSocket socket(ctx, Udp::v4());
    socket.set_option(boost::asio::socket_base::reuse_address{true});
    socket.bind(UdpEndpoint(Udp::v4(), Config::cfg.portUdp));
    return socket;
  }

  TraderSink &mSink;

  OrderSocket mOrderSocket;
  OrderSocket mStatusSocket;
  PriceSocket mPriceSocket;
};

} // namespace hft::trader

#endif // HFT_COMMON_NETWORK_SERVER_HPP