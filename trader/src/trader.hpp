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
#include "control_center.hpp"
#include "db/postgres_adapter.hpp"
#include "market_types.hpp"
#include "network/async_socket.hpp"
#include "network_types.hpp"
#include "rtt_tracker.hpp"
#include "template_types.hpp"
#include "trader_bus.hpp"
#include "trader_command.hpp"
#include "types.hpp"
#include "utils/market_utils.hpp"
#include "utils/rng.hpp"
#include "utils/utils.hpp"

namespace hft::trader {

class Trader {
  using TraderTcpSocket = AsyncSocket<TcpSocket, TraderBus, OrderStatus>;
  using TraderUdpSocket = AsyncSocket<UdpSocket, TraderBus, TickerPrice>;
  using TraderControlCenter = ControlCenter<TraderCommand>;
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
        controlCenter_{ioCtx_, TraderBus::systemBus}, prices_{db::PostgresAdapter::readTickers()},
        tradeTimer_{ioCtx_}, statsTimer_{ioCtx_}, connectionTimer_{ioCtx_},
        tradeRate_{Config::cfg.tradeRate}, monitorRate_{Config::cfg.monitorRate} {
    utils::unblockConsole();

    Logger::monitorLogger->info(std::format("Market data loaded for {} tickers", prices_.size()));

    // Socket status events
    TraderBus::systemBus.subscribe<SocketStatusEvent>(
        [this](CRef<SocketStatusEvent> event) { onSocketStatus(event); });

    // OrderStatus events
    TraderBus::marketBus.setHandler<OrderStatus>([this](Span<OrderStatus> events) {
      for (auto &status : events) {
        onOrderStatus(status);
      }
    });

    // TickerPrice events
    TraderBus::marketBus.setHandler<TickerPrice>([this](Span<TickerPrice> prices) {
      for (auto &price : prices) {
        onPriceUpdate(price);
      }
    });

    // Subscribe to TraderCommands
    TraderBus::systemBus.subscribe(TraderCommand::Shutdown, [this]() { stop(); });
    TraderBus::systemBus.subscribe(TraderCommand::TradeStart, [this]() { tradeStart(); });
    TraderBus::systemBus.subscribe(TraderCommand::TradeStop, [this]() { tradeStop(); });
    TraderBus::systemBus.subscribe(TraderCommand::TradeSpeedUp, [this]() {
      if (tradeRate_ > Microseconds(1)) {
        tradeRate_ /= 2;
        Logger::monitorLogger->info(std::format("Trade rate: {}", tradeRate_));
      }
    });
    TraderBus::systemBus.subscribe(TraderCommand::TradeSpeedDown, [this]() {
      tradeRate_ *= 2;
      Logger::monitorLogger->info(std::format("Trade rate: {}", tradeRate_));
    });

    controlCenter_.addCommand("t+", TraderCommand::TradeStart);
    controlCenter_.addCommand("t-", TraderCommand::TradeStop);
    controlCenter_.addCommand("ts+", TraderCommand::TradeSpeedUp);
    controlCenter_.addCommand("ts-", TraderCommand::TradeSpeedDown);
    controlCenter_.addCommand("q", TraderCommand::Shutdown);

    controlCenter_.printCommands();
  }

  void start() {
    utils::setTheadRealTime();
    scheduleConnectionTimer();

    controlCenter_.start();

    ingressSocket_.asyncConnect();
    egressSocket_.asyncConnect();
    pricesSocket_.asyncConnect();

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

  void onOrderStatus(CRef<OrderStatus> status) {
    spdlog::debug("OrderStatus {}", [&status] { return utils::toString(status); }());
    Tracker::logRtt(status.id);
  }

  void onPriceUpdate(CRef<TickerPrice> price) {
    spdlog::debug([&price] { return utils::toString(price); }());
  }

  void tradeStart() {
    if (ingressSocket_.status() != SocketStatus::Connected ||
        egressSocket_.status() != SocketStatus::Connected) {
      Logger::monitorLogger->error("Not connected to the server");
      return;
    }
    scheduleTradeTimer();
    scheduleStatsTimer();
  }

  void tradeStop() {
    tradeTimer_.cancel();
    statsTimer_.cancel();
  }

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

  void scheduleConnectionTimer() {
    connectionTimer_.expires_after(monitorRate_);
    connectionTimer_.async_wait([this](BoostErrorRef ec) {
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
      }
      scheduleConnectionTimer();
    });
  }

  void scheduleStatsTimer() {
    statsTimer_.expires_after(monitorRate_);
    statsTimer_.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        return;
      }
      Tracker::printStats();
      scheduleStatsTimer();
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
    order.price = utils::fluctuateThePrice(tickerPrice.price);
    order.action = utils::RNG::rng(1) == 0 ? OrderAction::Buy : OrderAction::Sell;
    order.quantity = utils::RNG::rng(100);
    spdlog::trace("Placing order {}", [&order] { return utils::toString(order); }());
    egressSocket_.asyncWrite(Span<Order>{&order, 1});
  }

private:
  IoContext ioCtx_;
  ContextGuard ioCtxGuard_;

  TraderTcpSocket ingressSocket_;
  TraderTcpSocket egressSocket_;
  TraderUdpSocket pricesSocket_;

  TraderControlCenter controlCenter_;

  std::vector<TickerPrice> prices_;
  SteadyTimer tradeTimer_;
  SteadyTimer statsTimer_;
  SteadyTimer connectionTimer_;

  Microseconds tradeRate_;
  Seconds monitorRate_;
};

} // namespace hft::trader

#endif // HFT_SERVER_SERVER_HPP