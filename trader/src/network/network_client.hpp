/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_TRADER_NETWORKCLIENT_HPP
#define HFT_TRADER_NETWORKCLIENT_HPP

#include <format>
#include <memory>
#include <spdlog/spdlog.h>
#include <thread>
#include <vector>

#include "boost_types.hpp"
#include "bus/bus.hpp"
#include "bus/subscription_holder.hpp"
#include "comparators.hpp"
#include "config/config.hpp"
#include "logging.hpp"
#include "market_types.hpp"
#include "network/socket_status.hpp"
#include "network/transport/tcp_transport.hpp"
#include "network/transport/udp_transport.hpp"
#include "network_types.hpp"
#include "serialization/flat_buffers/fb_serializer.hpp"
#include "template_types.hpp"
#include "trader_command.hpp"
#include "trader_events.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft::trader {

/**
 * @brief
 */
class NetworkClient {
  using TraderTcpTransport = TcpTransport<Bus>;
  using TraderUdpTransport = UdpTransport<>;

public:
  NetworkClient(Bus &bus)
      : guard_{MakeGuard(ioCtx_.get_executor())}, bus_{bus},
        upstreamTransport_{createUpstreamTransport()},
        downstreamTransport_{createDownstreamTransport()},
        pricesTransport_{createPricesTransport()}, connectionTimer_{ioCtx_},
        monitorRate_{Config::cfg.monitorRate} {}

  ~NetworkClient() { stop(); }

  void start() {
    const auto cores = Config::cfg.coresNetwork.size();
    LOG_INFO_SYSTEM("Starting network client on {} threads", cores);
    workerThreads_.reserve(cores);
    for (int i = 0; i < cores; ++i) {
      workerThreads_.emplace_back([this, i]() {
        try {
          auto coreId = Config::cfg.coresNetwork[i];
          utils::setTheadRealTime(coreId);
          utils::pinThreadToCore(coreId);
          ioCtx_.run();
        } catch (const std::exception &e) {
          LOG_ERROR_SYSTEM("Exception in network thread {}", e.what());
        }
      });
    }
    ioCtx_.post([this]() {
      LOG_INFO_SYSTEM("Connecting to the server");
      scheduleConnectionTimer();
      upstreamTransport_.connect();
      downstreamTransport_.connect();
      pricesTransport_.read();
    });
  }

  void stop() { ioCtx_.stop(); }

  inline bool connected() const { return connected_.load(); }

private:
  void scheduleConnectionTimer() {
    connectionTimer_.expires_after(monitorRate_);
    connectionTimer_.async_wait([this](CRef<BoostError> ec) {
      if (ec) {
        LOG_ERROR_SYSTEM("{}", ec.message());
        return;
      }
      const bool ingressOn = upstreamTransport_.status() == SocketStatus::Connected;
      const bool egressOn = downstreamTransport_.status() == SocketStatus::Connected;
      if (!ingressOn || !egressOn) {
        LOG_ERROR_SYSTEM("Server is down, reconnecting...");
        if (!ingressOn) {
          upstreamTransport_.reconnect();
        }
        downstreamTransport_.reconnect();
        scheduleConnectionTimer();
      }
    });
  }

  TraderTcpTransport createUpstreamTransport() {
    return TraderTcpTransport{
        TcpSocket{ioCtx_}, TcpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portTcpOut},
        bus_};
  }

  TraderTcpTransport createDownstreamTransport() {
    return TraderTcpTransport{TcpSocket{ioCtx_},
                              TcpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portTcpIn},
                              bus_};
  }

  TraderUdpTransport createPricesTransport() {
    return TraderUdpTransport{utils::createUdpSocket(ioCtx_, false, Config::cfg.portUdp),
                              UdpEndpoint(Udp::v4(), Config::cfg.portUdp), bus_};
  }

private:
  IoCtx ioCtx_;
  ContextGuard guard_;

  Bus &bus_;

  TraderTcpTransport upstreamTransport_;
  TraderTcpTransport downstreamTransport_;
  TraderUdpTransport pricesTransport_;

  Opt<Token> token_;

  SteadyTimer connectionTimer_;
  Seconds monitorRate_;

  std::atomic_bool connected_{false};
  std::vector<Thread> workerThreads_;
};

} // namespace hft::trader

#endif // HFT_TRADER_NETWORKCLIENT_HPP
