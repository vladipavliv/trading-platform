/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_HFTSERVER_HPP
#define HFT_SERVER_HFTSERVER_HPP

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
#include "network_types.hpp"
#include "order_book.hpp"
#include "server_tcp_socket.hpp"
#include "template_types.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft {

class Server {
  using Socket = ServerTcpSocket<Order>;
  using UPtrSocket = std::unique_ptr<Socket>;
  using UPtrOrderBook = std::unique_ptr<FlatOrderBook>;

  struct Session {
    UPtrSocket ingress;
    UPtrSocket egress;
  };

public:
  Server() : mIngressAcceptor{mCtx}, mEgressAcceptor{mCtx}, mTimer{mCtx} {
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    std::cout << std::unitbuf;

    initMarketData();
    startWorkers();
    startIngress();
    startEgress();
    scheduleTimer();
  }
  ~Server() {
    for (auto &thread : mWorkerThreads) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

  void start() { mCtx.run(); }
  void stop() { mCtx.stop(); }

private:
  void startIngress() {
    TcpEndpoint endpoint(Tcp::v4(), Config::cfg.portTcpIn);
    mIngressAcceptor.open(endpoint.protocol());
    mIngressAcceptor.bind(endpoint);
    mIngressAcceptor.listen();
    acceptIngress();
  }

  void acceptIngress() {
    mIngressAcceptor.async_accept([this](BoostErrorRef ec, TcpSocket socket) {
      socket.set_option(TcpSocket::protocol_type::no_delay(true));
      if (ec) {
        spdlog::error("Failed to accept connection {}", ec.message());
        return;
      }
      auto traderId = utils::getTraderId(socket);
      Logger::monitorLogger->info("{} connected", traderId);
      mSessions[traderId].ingress = std::make_unique<Socket>(
          std::move(socket), traderId,
          [this, traderId](const Order &order) { dispatchOrder(traderId, order); });
      mSessions[traderId].ingress->asyncRead();
      acceptIngress();
    });
  }

  void startEgress() {
    TcpEndpoint endpoint(Tcp::v4(), Config::cfg.portTcpOut);
    mEgressAcceptor.open(endpoint.protocol());
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
      auto id = utils::getTraderId(socket);
      Logger::monitorLogger->info("{} connected", id);
      mSessions[id].egress = std::make_unique<Socket>(std::move(socket), id, [](const Order &) {});
      acceptEgress();
    });
  }

  void startWorkers() {
    mWorkerContexts.reserve(Config::cfg.coresApp.size());
    mWorkerGuards.reserve(Config::cfg.coresApp.size());
    for (int i = 0; i < Config::cfg.coresApp.size(); ++i) {
      mWorkerContexts.emplace_back(std::make_unique<IoContext>());
      mWorkerGuards.emplace_back(
          std::make_unique<ContextGuard>(boost::asio::make_work_guard(*mWorkerContexts.back())));
      mWorkerThreads.emplace_back([this, i]() {
        try {
          mWorkerContexts[i]->run();
        } catch (const std::exception &e) {
          Logger::monitorLogger->error("Exception in worker thread {}", e.what());
        }
      });
    }
  }

  size_t getTicketHash(const Ticker &ticker) {
    return std::hash<std::string_view>{}(std::string_view(ticker.data(), ticker.size()));
  }

  ThreadId getWorkerId(const Ticker &ticker) {
    return getTicketHash(ticker) % Config::cfg.coresApp.size();
  }

  void dispatchOrder(TraderId traderId, const Order &order) {
    mOrdersTotal.fetch_add(1, std::memory_order_relaxed);
    ThreadId workerId = getWorkerId(order.ticker);
    boost::asio::post(*mWorkerContexts[workerId], [this, order, traderId]() {
      size_t tickerId = getTicketHash(order.ticker);
      mOrderBooks[tickerId]->add(order);
      auto matches = mOrderBooks[tickerId]->match();
      mSessions[traderId].egress->asyncWrite(Span<OrderStatus>(matches));
      mOrdersClosed.fetch_add(matches.size(), std::memory_order_relaxed);
    });
  }

  void initMarketData() {
    auto tickers = db::PostgresAdapter::readTickers();
    for (auto &item : tickers) {
      mOrderBooks[getTicketHash(item.ticker)] = std::make_unique<FlatOrderBook>();
    }
  }

  void scheduleTimer() {
    mTimer.expires_after(Seconds(1));
    mTimer.async_wait([this](BoostErrorRef ec) {
      if (ec) {
        return;
      }
      checkInput();
      Logger::monitorLogger->info("Orders [matched|total] {} {}",
                                  mOrdersClosed.load(std::memory_order_relaxed),
                                  mOrdersTotal.load(std::memory_order_relaxed));
      scheduleTimer();
    });
  }

  void checkInput() {
    struct pollfd fds = {STDIN_FILENO, POLLIN, 0};
    if (poll(&fds, 1, 0) == 1) {
      std::string cmd;
      std::getline(std::cin, cmd);
      if (cmd == "q") {
        mCtx.stop();
        for (auto &ctx : mWorkerContexts) {
          ctx->stop();
        }
      }
    }
  }

private:
  IoContext mCtx;

  TcpAcceptor mIngressAcceptor;
  TcpAcceptor mEgressAcceptor;
  SteadyTimer mTimer;

  std::vector<UPtrIoContext> mWorkerContexts;
  std::vector<UPtrContextGuard> mWorkerGuards;
  std::vector<std::thread> mWorkerThreads;

  std::unordered_map<size_t, Session> mSessions;
  std::unordered_map<size_t, UPtrOrderBook> mOrderBooks;

  std::atomic_size_t mOrdersTotal;
  std::atomic_size_t mOrdersClosed;
};

} // namespace hft

#endif // HFT_SERVER_HFTSERVER_HPP