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
        mOrderSocket{sink, TcpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portTcpIn}},
        mStatusSocket{sink, TcpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portTcpOut}},
        mPriceSocket{sink, UdpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portUdp}} {}

  void start() {
    mOrderSocket.asyncConnect();
    mStatusSocket.asyncConnect();
    mPriceSocket.asyncConnect();

    mSink.networkSink.setHandler<Order>(
        [this](const Order &order) { mOrderSocket.asyncWrite(order); });
    mSink.dataSink.setHandler<OrderStatus>(
        [this](const OrderStatus &status) { spdlog::debug(utils::toString(status)); });
    mSink.dataSink.setHandler<PriceUpdate>(
        [this](const PriceUpdate &price) { spdlog::debug(utils::toString(price)); });
  }

  void stop() {
    mOrderSocket.close();
    mStatusSocket.close();
    mPriceSocket.close();
  }

private:
  TraderSink &mSink;

  OrderSocket mOrderSocket;
  OrderSocket mStatusSocket;
  PriceSocket mPriceSocket;
};

} // namespace hft::trader

#endif // HFT_COMMON_NETWORK_SERVER_HPP