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
#include "network/async_tcp_socket.hpp"
#include "network_types.hpp"
#include "rtt_tracker.hpp"
#include "template_types.hpp"
#include "types.hpp"
#include "utils/rng.hpp"
#include "utils/utils.hpp"

namespace hft::trader {

class Trader {
  using TraderTcpSocket = AsyncTcpSocket<OrderStatus>;

public:
  Trader()
      : mGuard{boost::asio::make_work_guard(mCtx)},
        mIngressSocket{TcpSocket{mCtx},
                       TcpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portTcpOut},
                       [this](const OrderStatus &status) { onOrderStatus(status); }},
        mEgressSocket{TcpSocket{mCtx},
                      TcpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portTcpIn},
                      [this](const OrderStatus &status) { onOrderStatus(status); }},
        mPrices{db::PostgresAdapter::readTickers()}, mTradeTimer{mCtx}, mMonitorTimer{mCtx},
        mTradeRate{Config::cfg.tradeRateUs}, mMonitorRate{Config::cfg.monitorRateS} {
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    std::cout << std::unitbuf;

    mIngressSocket.asyncConnect([this]() {
      Logger::monitorLogger->info("Ingress socket connected");
      mIngressSocket.asyncRead();
    });
    mEgressSocket.asyncConnect(
        [this]() { Logger::monitorLogger->info("Egress socket connected"); });
    scheduleMonitorTimer();

    Logger::monitorLogger->info(std::format("Market data loaded for {} tickers", mPrices.size()));
  }

  void start() {
    utils::setTheadRealTime();
    mCtx.run();
  }
  void stop() { mCtx.stop(); }

private:
  size_t getTicketHash(const Ticker &ticker) {
    return std::hash<std::string_view>{}(std::string_view(ticker.data(), ticker.size()));
  }

  ThreadId getWorkerId(const Ticker &ticker) {
    return getTicketHash(ticker) % Config::cfg.coreIds.size();
  }

  void onOrderStatus(const OrderStatus &status) {
    spdlog::debug("Order status {}", [&status] { return utils::toString(status); }());
    RttTracker::logRtt(status.id);
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
      checkInput();
      showStats();
      scheduleMonitorTimer();
    });
  }

  void showStats() {
    using namespace utils;
    auto rtt = RttTracker::getStats().samples;
    auto sampleSize = rtt[0].size + rtt[1].size + rtt[2].size;
    auto s0Rate = ((float)rtt[0].size / sampleSize) * 100;
    auto s1Rate = ((float)rtt[1].size / sampleSize) * 100;
    auto s2Rate = ((float)rtt[2].size / sampleSize) * 100;

    if (sampleSize == 0) {
      return;
    }
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "RTT [1us|100us|1ms]  " << s0Rate << "% avg:";
    ss << ((rtt[0].size != 0) ? (rtt[0].sum / rtt[0].size) : 0);
    ss << "us  " << s1Rate << "% avg:";
    ss << ((rtt[1].size != 0) ? (rtt[1].sum / rtt[1].size) : 0);
    ss << "us  " << s2Rate << "% avg:";
    ss << ((rtt[2].size != 0) ? ((rtt[2].sum / rtt[2].size) / 1000) : 0);
    ss << "ms";

    Logger::monitorLogger->info(ss.str());
  }

  void tradeSomething() {
    auto tickerPrice = mPrices[utils::RNG::rng(mPrices.size() - 1)];
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
        scheduleTradeTimer();
      } else if (cmd == "t-") {
        mTradeTimer.cancel();
      } else if (cmd == "ts-") {
        mTradeRate *= 2;
        Logger::monitorLogger->info(std::format("Trade rate: {}us", mTradeRate));
      } else if (cmd == "ts+" && mTradeRate > Microseconds(10)) {
        mTradeRate /= 2;
        Logger::monitorLogger->info(std::format("Trade rate: {}us", mTradeRate));
      }
    }
  }

private:
  IoContext mCtx;
  ContextGuard mGuard;

  TraderTcpSocket mIngressSocket;
  TraderTcpSocket mEgressSocket;

  std::vector<TickerPrice> mPrices;
  SteadyTimer mTradeTimer;
  SteadyTimer mMonitorTimer;

  Microseconds mTradeRate;
  Seconds mMonitorRate;
};

} // namespace hft::trader

#endif // HFT_SERVER_SERVER_HPP