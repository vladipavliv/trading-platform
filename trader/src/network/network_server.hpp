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
      : mSink{sink}, mCfg{Config::config()},
        mOrderSocket{sink, TcpEndpoint{Ip::make_address(mCfg.trader.url), mCfg.trader.portTcpIn}},
        mStatusSocket{sink, TcpEndpoint{Ip::make_address(mCfg.trader.url), mCfg.trader.portTcpOut}},
        mPriceSocket{sink, UdpEndpoint{Ip::address_v4::any(), mCfg.trader.portUdp}} {}

  void start() {}

  void stop() {}

private:
  TraderSink &mSink;
  const Config &mCfg;

  OrderSocket mOrderSocket;
  OrderSocket mStatusSocket;
  PriceSocket mPriceSocket;
};

} // namespace hft::trader

#endif // HFT_COMMON_NETWORK_SERVER_HPP