/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_SERVER_HPP
#define HFT_SERVER_SERVER_HPP

#include <fcntl.h>
#include <format>
#include <iostream>
#include <memory>
#include <unordered_map>

#include "boost_types.hpp"
#include "comparators.hpp"
#include "config/config.hpp"
#include "db/postgres_adapter.hpp"
#include "market_types.hpp"
#include "network/async_socket.hpp"
#include "network_types.hpp"
#include "rtt_tracker.hpp"
#include "template_types.hpp"
#include "types.hpp"
#include "utils/rng.hpp"
#include "utils/utils.hpp"

namespace hft::trader {

class Trader {
  using TraderTcpSocket = AsyncSocket<TcpSocket, OrderStatus>;
  using TraderUdpSocket = AsyncSocket<UdpSocket, TickerPrice>;
  using Tracker = RttTracker<50, 200>;

public:
  Trader()
      : mGuard{boost::asio::make_work_guard(mCtx)},
        mIngressSocket{TcpSocket{mCtx},
                       TcpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portTcpOut},
                       [this](const OrderStatus &status) { onOrderStatus(status); }},
        mEgressSocket{TcpSocket{mCtx},
                      TcpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portTcpIn}},
        mPricesSocket{createUdpSocket(), UdpEndpoint(Udp::v4(), Config::cfg.portUdp),
                      [this](const TickerPrice &priceUpdate) { onPriceUpdate(priceUpdate); }},
        mPrices{db::PostgresAdapter::readTickers()}, mTradeTimer{mCtx}, mMonitorTimer{mCtx},
        mInputTimer{mCtx}, mTradeRate{Config::cfg.tradeRateUs},
        mMonitorRate{Config::cfg.monitorRateS} {
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    std::cout << std::unitbuf;

    mIngressSocket.asyncConnect([this]() {
      Logger::monitorLogger->info("Ingress socket connected");
      mIngressSocket.asyncRead();
    });
    mEgressSocket.asyncConnect(
        [this]() { Logger::monitorLogger->info("Egress socket connected"); });
    mPricesSocket.asyncConnect();

    Logger::monitorLogger->info(std::format("Market data loaded for {} tickers", mPrices.size()));
    scheduleInputTimer();
  }

  void start() {
    utils::setTheadRealTime();
    mCtx.run();
  }
  void stop() { mCtx.stop(); }

private:
  void onOrderStatus(const OrderStatus &status) {
    spdlog::debug("OrderStatus {}", [&status] { return utils::toString(status); }());
    Tracker::logRtt(status.id);
  }

  void onPriceUpdate(const TickerPrice &price) {
    spdlog::debug([&price] { return utils::toString(price); }());
  }

  void tradeStart() {
    scheduleTradeTimer();
    scheduleMonitorTimer();
  }

  void tradeStop() {
    mTradeTimer.cancel();
    mMonitorTimer.cancel();
  }

  void scheduleTradeTimer() {
    mTradeTimer.expires_after(mTradeRate);
    mTradeTimer.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        return;
      }
      tradeSomething();
      scheduleTradeTimer();
    });
  }

  void scheduleMonitorTimer() {
    mMonitorTimer.expires_after(mMonitorRate);
    mMonitorTimer.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        return;
      }
      Tracker::printStats();
      scheduleMonitorTimer();
    });
  }

  void scheduleInputTimer() {
    mInputTimer.expires_after(Milliseconds(200));
    mInputTimer.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        return;
      }
      checkInput();
      scheduleInputTimer();
    });
  }

  void tradeSomething() {
    static auto cursor = mPrices.begin();
    if (cursor == mPrices.end()) {
      cursor = mPrices.begin();
    }
    auto tickerPrice = *cursor++;
    Order order;
    order.id = utils::getLinuxTimestamp();
    order.ticker = tickerPrice.ticker;
    order.price = utils::RNG::rng<uint32_t>(tickerPrice.price * 2);
    order.action = utils::RNG::rng(1) == 0 ? OrderAction::Buy : OrderAction::Sell;
    order.quantity = utils::RNG::rng(1000);
    spdlog::trace("Placing order {}", [&order] { return utils::toString(order); }());
    mEgressSocket.asyncWrite(Span<Order>{&order, 1});
  }

  void checkInput() {
    struct pollfd fds = {STDIN_FILENO, POLLIN, 0};
    if (poll(&fds, 1, 0) == 1) {
      std::string cmd;
      std::getline(std::cin, cmd);
      if (cmd == "q") {
        stop();
      } else if (cmd == "t+") {
        tradeStart();
      } else if (cmd == "t-") {
        tradeStop();
      } else if (cmd == "ts-") {
        mTradeRate *= 2;
        Logger::monitorLogger->info(std::format("Trade rate: {}", mTradeRate));
      } else if (cmd == "ts+" && mTradeRate > Microseconds(10)) {
        mTradeRate /= 2;
        Logger::monitorLogger->info(std::format("Trade rate: {}", mTradeRate));
      }
    }
  }

  UdpSocket createUdpSocket() {
    UdpSocket socket(mCtx, Udp::v4());
    socket.set_option(boost::asio::socket_base::reuse_address{true});
    socket.bind(UdpEndpoint(Udp::v4(), Config::cfg.portUdp));
    return socket;
  }

private:
  IoContext mCtx;
  ContextGuard mGuard;

  TraderTcpSocket mIngressSocket;
  TraderTcpSocket mEgressSocket;
  TraderUdpSocket mPricesSocket;

  std::vector<TickerPrice> mPrices;
  SteadyTimer mTradeTimer;
  SteadyTimer mMonitorTimer;
  SteadyTimer mInputTimer;

  Microseconds mTradeRate;
  Seconds mMonitorRate;
};

} // namespace hft::trader

#endif // HFT_SERVER_SERVER_HPP