/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_SERVER_HPP
#define HFT_SERVER_SERVER_HPP

#include <format>
#include <memory>
#include <unordered_map>

#include "boost_types.hpp"
#include "comparators.hpp"
#include "config/config.hpp"
#include "db/postgres_adapter.hpp"
#include "market_types.hpp"
#include "network/async_socket.hpp"
#include "network_types.hpp"
#include "order_book.hpp"
#include "template_types.hpp"
#include "types.hpp"
#include "utils/utils.hpp"
#include "worker.hpp"

namespace hft::server {

class Server {
  using ServerTcpSocket = AsyncSocket<TcpSocket, Order>;
  using ServerUdpSocket = AsyncSocket<UdpSocket, TickerPrice>;
  using OrderBook = FlatOrderBook;

  struct Session {
    ServerTcpSocket::UPtr ingress;
    ServerTcpSocket::UPtr egress;
  };

public:
  Server()
      : mIngressAcceptor{mCtx}, mEgressAcceptor{mCtx},
        mPricesSocket{utils::createUdpSocket(mCtx),
                      UdpEndpoint{Ip::address_v4::broadcast(), Config::cfg.portUdp},
                      [](const TickerPrice &) {}, [](SocketStatus status) {}},
        mInputTimer{mCtx}, mStatsTimer{mCtx}, mPriceTimer{mCtx},
        mStatsRateS{Config::cfg.monitorRateS}, mPriceRateUs{Config::cfg.priceFeedRateUs} {
    if (Config::cfg.coreIds.size() == 0 || Config::cfg.coreIds.size() > 10) {
      throw std::runtime_error("Invalid cores configuration");
    }
    utils::unblockConsole();

    initMarketData();
    startIngress();
    startEgress();
    startWorkers();
    scheduleInputTimer();
    scheduleStatsTimer();
  }
  ~Server() { stop(); }

  void start() {
    utils::setTheadRealTime();
    mCtx.run();
  }

  void stop() {
    mIngressAcceptor.close();
    mEgressAcceptor.close();
    mCtx.stop();
    for (auto &worker : mWorkers) {
      worker->stop();
    }
  }

private:
  void startIngress() {
    TcpEndpoint endpoint(Tcp::v4(), Config::cfg.portTcpIn);
    mIngressAcceptor.open(endpoint.protocol());
    mIngressAcceptor.set_option(boost::asio::socket_base::reuse_address(true));
    mIngressAcceptor.bind(endpoint);
    mIngressAcceptor.listen();
    acceptIngress();
  }

  void acceptIngress() {
    mIngressAcceptor.async_accept([this](BoostErrorRef ec, TcpSocket socket) {
      if (ec) {
        spdlog::error("Failed to accept connection {}", ec.message());
        return;
      }
      socket.set_option(TcpSocket::protocol_type::no_delay(true));
      auto traderId = utils::getTraderId(socket);
      Logger::monitorLogger->info("{} connected ingress", traderId);
      mSessions[traderId].ingress = std::make_unique<ServerTcpSocket>(
          std::move(socket),
          [this, traderId](const Order &order) { dispatchOrder(traderId, order); },
          [this, traderId](SocketStatus status) { onSocketStatus(traderId, status); }, traderId);
      mSessions[traderId].ingress->asyncRead();
      acceptIngress();
    });
  }

  void startEgress() {
    TcpEndpoint endpoint(Tcp::v4(), Config::cfg.portTcpOut);
    mEgressAcceptor.open(endpoint.protocol());
    mEgressAcceptor.set_option(boost::asio::socket_base::reuse_address(true));
    mEgressAcceptor.bind(endpoint);
    mEgressAcceptor.listen();
    acceptEgress();
  }

  void acceptEgress() {
    mEgressAcceptor.async_accept([this](BoostErrorRef ec, TcpSocket socket) {
      if (ec) {
        spdlog::error("Failed to accept connection: {}", ec.message());
        return;
      }
      socket.set_option(TcpSocket::protocol_type::no_delay(true));
      auto traderId = utils::getTraderId(socket);
      Logger::monitorLogger->info("{} connected egress", traderId);
      mSessions[traderId].egress = std::make_unique<ServerTcpSocket>(
          std::move(socket), [](const Order &) {},
          [this, traderId](SocketStatus status) { onSocketStatus(traderId, status); }, traderId);
      acceptEgress();
    });
  }

  void startWorkers() {
    mWorkers.reserve(Config::cfg.coreIds.size());
    for (int i = 0; i < Config::cfg.coreIds.size(); ++i) {
      mWorkers.emplace_back(std::make_unique<Worker>(i));
    }
  }

  ThreadId getWorkerId(const Ticker &ticker) {
    return utils::getTickerHash(ticker) % Config::cfg.coreIds.size();
  }

  void dispatchOrder(TraderId traderId, const Order &order) {
    spdlog::debug([&order] { return utils::toString(order); }());
    mOrdersTotal.fetch_add(1, std::memory_order_relaxed);

    ThreadId workerId = getWorkerId(order.ticker);
    mWorkers[workerId]->post([this, order, traderId]() {
      size_t tickerId = utils::getTickerHash(order.ticker);
      mOrderBooks[tickerId].add(order);
      auto matches = mOrderBooks[tickerId].match();
      mSessions[traderId].egress->asyncWrite(Span<OrderStatus>(matches));
      mOrdersClosed.fetch_add(matches.size(), std::memory_order_relaxed);
    });
  }

  void onSocketStatus(TraderId id, SocketStatus status) {
    switch (status) {
    case SocketStatus::Disconnected:
      Logger::monitorLogger->info("{} disconnected", id);
      mSessions.erase(id);
      break;
    default:
      break;
    }
  }

  void initMarketData() {
    mPrices = db::PostgresAdapter::readTickers();
    for (auto &item : mPrices) {
      mOrderBooks.insert({utils::getTickerHash(item.ticker), OrderBook{}});
    }
    Logger::monitorLogger->info(std::format("Market data loaded for {} tickers", mPrices.size()));
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

  void scheduleStatsTimer() {
    mStatsTimer.expires_after(Seconds(mStatsRateS));
    mStatsTimer.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        return;
      }
      static size_t lastOrderCount = 0;
      size_t ordersCurrent = mOrdersTotal.load(std::memory_order_relaxed);
      auto rps = (ordersCurrent - lastOrderCount) / mStatsRateS;
      if (rps != 0) {
        Logger::monitorLogger->info("Orders [matched|total] {} {} rps:{}",
                                    mOrdersClosed.load(std::memory_order_relaxed),
                                    mOrdersTotal.load(std::memory_order_relaxed), rps);
      }
      lastOrderCount = ordersCurrent;
      scheduleStatsTimer();
    });
  }

  void schedulePriceTimer() {
    mPriceTimer.expires_after(Microseconds(mPriceRateUs));
    mPriceTimer.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        return;
      }
      updatePrices();
      schedulePriceTimer();
    });
  }

  void updatePrices() {
    static auto cursor = mPrices.begin();
    const auto pricesPerUpdate = 5;
    std::vector<TickerPrice> priceUpdates;
    priceUpdates.reserve(pricesPerUpdate);
    for (int i = 0; i < pricesPerUpdate; ++i) {
      if (cursor == mPrices.end()) {
        cursor = mPrices.begin();
      }
      auto tickerPrice = *cursor++;
      tickerPrice.price = utils::getLinuxTimestamp() % 777;
      priceUpdates.emplace_back(std::move(tickerPrice));
      spdlog::trace([&tickerPrice] { return utils::toString(tickerPrice); }());
    }
    mPricesSocket.asyncWrite(Span<TickerPrice>(priceUpdates));
  }

  void checkInput() {
    auto cmd = utils::getConsoleInput();
    if (cmd.empty()) {
      return;
    } else if (cmd == "q") {
      stop();
    } else if (cmd == "p+") {
      schedulePriceTimer();
      Logger::monitorLogger->info("Price feed start");
    } else if (cmd == "p-") {
      mPriceTimer.cancel();
      Logger::monitorLogger->info("Price feed stop");
    }
  }

private:
  IoContext mCtx;

  TcpAcceptor mIngressAcceptor;
  TcpAcceptor mEgressAcceptor;
  ServerUdpSocket mPricesSocket;

  SteadyTimer mInputTimer;
  SteadyTimer mStatsTimer;
  SteadyTimer mPriceTimer;

  size_t mStatsRateS;
  size_t mPriceRateUs;

  std::vector<Worker::UPtr> mWorkers;

  std::unordered_map<size_t, Session> mSessions;
  std::unordered_map<size_t, OrderBook> mOrderBooks;
  std::vector<TickerPrice> mPrices;

  std::atomic_size_t mOrdersTotal;
  std::atomic_size_t mOrdersClosed;
};

} // namespace hft::server

#endif // HFT_SERVER_SERVER_HPP