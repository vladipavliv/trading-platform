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
      : ioCtxGuard_{boost::asio::make_work_guard(ioCtx_)},
        ingressSocket_{TcpSocket{ioCtx_},
                       TcpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portTcpOut}},
        egressSocket_{TcpSocket{ioCtx_},
                      TcpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portTcpIn}},
        pricesSocket_{utils::createUdpSocket(ioCtx_, false, Config::cfg.portUdp),
                      UdpEndpoint(Udp::v4(), Config::cfg.portUdp)},
        prices_{db::PostgresAdapter::readTickers()}, tradeTimer_{ioCtx_}, monitorTimer_{ioCtx_},
        inputTimer_{ioCtx_}, tradeRate_{Config::cfg.tradeRateUs},
        monitorRate_{Config::cfg.monitorRateS} {
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

    ingressSocket_.asyncConnect();
    egressSocket_.asyncConnect();
    pricesSocket_.asyncConnect();

    Logger::monitorLogger->info(std::format("Market data loaded for {} tickers", prices_.size()));
    scheduleInputTimer();
    scheduleMonitorTimer();
  }

  void start() {
    utils::setTheadRealTime();
    ioCtx_.run();
  }

  void stop() { ioCtx_.stop(); }

private:
  void onSocketStatus(SocketStatusEvent event) {
    switch (event.status) {
    case SocketStatus::Connected:
      if (ingressSocket_.status() == SocketStatus::Connected &&
          egressSocket_.status() == SocketStatus::Connected) {
        Logger::monitorLogger->info("Connected to the server");
        ingressSocket_.asyncRead();
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
    if (ingressSocket_.status() != SocketStatus::Connected ||
        egressSocket_.status() != SocketStatus::Connected) {
      Logger::monitorLogger->error("Not connected to the server");
      return;
    }
    scheduleTradeTimer();
  }

  void tradeStop() { tradeTimer_.cancel(); }

  void scheduleTradeTimer() {
    tradeTimer_.expires_after(tradeRate_);
    tradeTimer_.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        return;
      }
      tradeSomething();
      scheduleTradeTimer();
    });
  }

  void scheduleMonitorTimer() {
    monitorTimer_.expires_after(monitorRate_);
    monitorTimer_.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        return;
      }
      const bool ingressOn = ingressSocket_.status() == SocketStatus::Connected;
      const bool egressOn = egressSocket_.status() == SocketStatus::Connected;
      if (!ingressOn || !egressOn) {
        Logger::monitorLogger->critical("Server is down, reconnecting...");
        if (!ingressOn) {
          ingressSocket_.reconnect();
        }
        // Egress socket won't passively notify about disconnect
        egressSocket_.reconnect();
      } else {
        Tracker::printStats();
      }
      scheduleMonitorTimer();
    });
  }

  void scheduleInputTimer() {
    inputTimer_.expires_after(Milliseconds(200));
    inputTimer_.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        return;
      }
      checkInput();
      scheduleInputTimer();
    });
  }

  void tradeSomething() {
    static auto cursor = prices_.begin();
    if (cursor == prices_.end()) {
      cursor = prices_.begin();
    }
    auto tickerPrice = *cursor++;
    Order order;
    order.id = utils::getLinuxTimestamp();
    order.ticker = tickerPrice.ticker;
    order.price = utils::RNG::rng<uint32_t>(tickerPrice.price * 2);
    order.action = utils::RNG::rng(1) == 0 ? OrderAction::Buy : OrderAction::Sell;
    order.quantity = utils::RNG::rng(1000);
    spdlog::trace("Placing order {}", [&order] { return utils::toString(order); }());
    egressSocket_.asyncWrite(Span<Order>{&order, 1});
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
      tradeRate_ *= 2;
      Logger::monitorLogger->info(std::format("Trade rate: {}", tradeRate_));
    } else if (cmd == "ts+" && tradeRate_ > Microseconds(1)) {
      tradeRate_ /= 2;
      Logger::monitorLogger->info(std::format("Trade rate: {}", tradeRate_));
    }
  }

private:
  IoContext ioCtx_;
  ContextGuard ioCtxGuard_;

  TraderTcpSocket ingressSocket_;
  TraderTcpSocket egressSocket_;
  TraderUdpSocket pricesSocket_;

  std::vector<TickerPrice> prices_;
  SteadyTimer tradeTimer_;
  SteadyTimer monitorTimer_;
  SteadyTimer inputTimer_;

  Microseconds tradeRate_;
  Seconds monitorRate_;
};

} // namespace hft::trader

#endif // HFT_SERVER_SERVER_HPP