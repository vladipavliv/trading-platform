/**
 * @file
 * @brief
 *
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
  using PriceSocket = TraderSocket<UdpSocket, PriceUpdate>;

  NetworkServer(TraderSink &sink)
      : mSink{sink},
        mOrderSocket{sink, TcpSocket{sink.ctx()},
                     TcpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portTcpIn}},
        mStatusSocket{sink, TcpSocket{sink.ctx()},
                      TcpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portTcpOut}},
        mPriceSocket{sink, createUdpSocket(sink.ctx()),
                     UdpEndpoint(Udp::v4(), Config::cfg.portUdp)} {}

  void start() {
    mOrderSocket.asyncConnect();
    mStatusSocket.asyncConnect();
    mPriceSocket.asyncConnect();

    mSink.networkSink.setHandler<Order>(
        [this](const Order &order) { mOrderSocket.asyncWrite(order); });
    mSink.dataSink.setHandler<OrderStatus>([this](const OrderStatus &status) {
      auto timestamp = utils::getLinuxTimestamp();
      auto RTT = timestamp - status.id;
      spdlog::debug("{} {}μs", utils::toString(status), RTT / 1000);
    });
    mSink.dataSink.setHandler<PriceUpdate>(
        [this](const PriceUpdate &price) { spdlog::debug(utils::toString(price)); });
  }

  void stop() {
    mOrderSocket.close();
    mStatusSocket.close();
    mPriceSocket.close();
  }

private:
  UdpSocket createUdpSocket(IoContext &ctx) {
    UdpSocket socket(ctx, Udp::v4());
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