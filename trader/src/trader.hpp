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
                       TcpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portTcpOut}},
        mEgressSocket{TcpSocket{mCtx},
                      TcpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portTcpIn}},
        mPricesSocket{utils::createUdpSocket(mCtx, false, Config::cfg.portUdp),
                      UdpEndpoint(Udp::v4(), Config::cfg.portUdp)},
        mPrices{db::PostgresAdapter::readTickers()}, mTradeTimer{mCtx}, mMonitorTimer{mCtx},
        mInputTimer{mCtx}, mTradeRate{Config::cfg.tradeRateUs},
        mMonitorRate{Config::cfg.monitorRateS} {
    utils::unblockConsole();

    EventBus::bus().subscribe<SocketStatusEvent>(
        [this](Span<SocketStatusEvent> events) { onSocketStatus(events.front()); });

    EventBus::bus().subscribe<OrderStatus>([this](Span<OrderStatus> events) {
      for (auto &status : events) {
        spdlog::debug("OrderStatus {}", [&status] { return utils::toString(status); }());
        Tracker::logRtt(status.id);
      }
    });

    EventBus::bus().subscribe<TickerPrice>([this](Span<TickerPrice> prices) {
      for (auto &price : prices) {
        spdlog::debug([&price] { return utils::toString(price); }());
      }
    });

    mIngressSocket.asyncConnect();
    mEgressSocket.asyncConnect();
    mPricesSocket.asyncConnect();

    Logger::monitorLogger->info(std::format("Market data loaded for {} tickers", mPrices.size()));
    scheduleInputTimer();
    scheduleMonitorTimer();
  }

  void start() {
    utils::setTheadRealTime();
    mCtx.run();
  }

  void stop() { mCtx.stop(); }

private:
  void onSocketStatus(SocketStatusEvent event) {
    switch (event.status) {
    case SocketStatus::Connected:
      if (mIngressSocket.status() == SocketStatus::Connected &&
          mEgressSocket.status() == SocketStatus::Connected) {
        Logger::monitorLogger->info("Connected to the server");
        mIngressSocket.asyncRead();
      }
      break;
    case SocketStatus::Disconnected:
      tradeStop();
      break;
    default:
      break;
    }
  }

  void tradeStart() {
    if (mIngressSocket.status() != SocketStatus::Connected ||
        mEgressSocket.status() != SocketStatus::Connected) {
      Logger::monitorLogger->error("Not connected to the server");
      return;
    }
    scheduleTradeTimer();
  }

  void tradeStop() { mTradeTimer.cancel(); }

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
      const bool ingressOn = mIngressSocket.status() == SocketStatus::Connected;
      const bool egressOn = mEgressSocket.status() == SocketStatus::Connected;
      if (!ingressOn || !egressOn) {
        Logger::monitorLogger->critical("Server is down, reconnecting...");
        if (!ingressOn) {
          mIngressSocket.reconnect();
        }
        // Egress socket won't passively notify about disconnect
        mEgressSocket.reconnect();
      } else {
        Tracker::printStats();
      }
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
    auto cmd = utils::getConsoleInput();
    if (cmd.empty()) {
      return;
    } else if (cmd == "q") {
      stop();
    } else if (cmd == "t+") {
      tradeStart();
    } else if (cmd == "t-") {
      tradeStop();
    } else if (cmd == "ts-") {
      mTradeRate *= 2;
      Logger::monitorLogger->info(std::format("Trade rate: {}", mTradeRate));
    } else if (cmd == "ts+" && mTradeRate > Microseconds(1)) {
      mTradeRate /= 2;
      Logger::monitorLogger->info(std::format("Trade rate: {}", mTradeRate));
    }
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