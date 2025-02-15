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

namespace hft::trader {

class NetworkServer {
public:
  using OrderSocket = TraderSocket<TcpSocket, OrderStatus>;
  using PriceSocket = TraderSocket<UdpSocket, PriceUpdate>;

  NetworkServer(TraderSink &sink)
      : mSink{sink}, mCfg{Config::cfg}, mOrderSocket{sink},
        mStatusSocket{sink} // mPriceSocket{sink}
  {}

  void start() {
    mOrderSocket.asyncConnect({Ip::make_address(mCfg.trader.url), mCfg.trader.portTcpOut});
    mStatusSocket.asyncConnect({Ip::make_address(mCfg.trader.url), mCfg.trader.portTcpIn});
    // mPriceSocket.asyncConnect({Ip::make_address(mCfg.trader.url), mCfg.trader.portUdp});

    mSink.networkSink.setHandler<Order>(
        [this](const Order &order) { mOrderSocket.asyncWrite(order); });
  }

  void stop() {
    mOrderSocket.close();
    mStatusSocket.close();
    // mPriceSocket.close();
  }

private:
  TraderSink &mSink;
  const Config &mCfg;

  OrderSocket mOrderSocket;
  OrderSocket mStatusSocket;
  // PriceSocket mPriceSocket;
};

} // namespace hft::trader

#endif // HFT_COMMON_NETWORK_SERVER_HPP