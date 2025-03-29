/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_NETWORKCLIENT_HPP
#define HFT_SERVER_NETWORKCLIENT_HPP

#include <format>
#include <memory>
#include <spdlog/spdlog.h>
#include <thread>
#include <vector>

#include "boost_types.hpp"
#include "bus/bus.hpp"
#include "comparators.hpp"
#include "config/config.hpp"
#include "logger.hpp"
#include "market_types.hpp"
#include "network/connection_status.hpp"
#include "network/transport/tcp_transport.hpp"
#include "network/transport/udp_transport.hpp"
#include "network_types.hpp"
#include "serialization/flat_buffers/fb_serializer.hpp"
#include "template_types.hpp"
#include "trader_command.hpp"
#include "trader_event.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft::trader {

/**
 * @brief Manages connections to the server
 * Runs a separate io_context on a number of threads, connects/reconnects
 */
class NetworkClient {
  using Serializer = serialization::FlatBuffersSerializer;
  using Framer = SizeFramer<Serializer>;
  using TraderTcpTransport = TcpTransport<Framer>;
  using TraderUdpTransport = UdpTransport<Framer>;

  struct Session {
    TraderTcpTransport::UPtr ingress;
    TraderTcpTransport::UPtr egress;
  };

public:
  NetworkClient(Bus &bus)
      : ioCtxGuard_{MakeGuard(ioCtx_.get_executor())}, bus_{bus},
        ingressTransport_{createIngressTransport()}, egressTransport_{createEgressTransport()},
        pricesTransport_{createPricesTransport()}, connectionTimer_{ioCtx_},
        monitorRate_{Config::cfg.monitorRate} {
    bus_.marketBus.setHandler<Order>(
        [this](Span<Order> orders) { egressTransport_.write(orders); });

    bus_.systemBus.subscribe<TcpConnectionStatus>(
        [this](CRef<TcpConnectionStatus> event) { onTcpStatus(event); });

    bus_.systemBus.subscribe<UdpConnectionStatus>(
        [this](CRef<UdpConnectionStatus> event) { onUdpStatus(event); });
  }

  ~NetworkClient() { stop(); }

  void start() {
    const auto cores = Config::cfg.coresNetwork.size();
    Logger::monitorLogger->info("Starting network client on {} threads", cores);
    workerThreads_.reserve(cores);
    for (int i = 0; i < cores; ++i) {
      workerThreads_.emplace_back([this, i]() {
        try {
          auto coreId = Config::cfg.coresNetwork[i];
          utils::setTheadRealTime(coreId);
          utils::pinThreadToCore(coreId);
          ioCtx_.run();
        } catch (const std::exception &e) {
          Logger::monitorLogger->error("Exception in network thread {}", e.what());
        }
      });
    }
    ioCtx_.post([this]() {
      Logger::monitorLogger->info("Connecting to the server");
      scheduleConnectionTimer();
      ingressTransport_.connect();
      egressTransport_.connect();
      pricesTransport_.read();
    });
  }

  void stop() { ioCtx_.stop(); }

  inline bool connected() const { return connected_.load(); }

private:
  void onTcpStatus(CRef<TcpConnectionStatus> event) {
    switch (event.status) {
    case ConnectionStatus::Connected:
      if (ingressTransport_.status() == ConnectionStatus::Connected &&
          egressTransport_.status() == ConnectionStatus::Connected) {
        if (!connected_) {
          connected_ = true;
          ingressTransport_.read();
          bus_.systemBus.post(TraderEvent::Connected);
        }
      }
      break;
    default:
      if (connected_) {
        connected_ = false;
        bus_.systemBus.post(TraderEvent::Disconnected);
      }
      scheduleConnectionTimer();
      break;
    }
  }

  void onUdpStatus(CRef<UdpConnectionStatus> event) {
    if (event.status == ConnectionStatus::Error) {
      Logger::monitorLogger->error("Udp connection error");
      pricesTransport_.close();
    }
  }

  void scheduleConnectionTimer() {
    connectionTimer_.expires_after(monitorRate_);
    connectionTimer_.async_wait([this](CRef<BoostError> ec) {
      if (ec) {
        return;
      }
      const bool ingressOn = ingressTransport_.status() == ConnectionStatus::Connected;
      const bool egressOn = egressTransport_.status() == ConnectionStatus::Connected;
      if (!ingressOn || !egressOn) {
        Logger::monitorLogger->critical("Server is down, reconnecting...");
        if (!ingressOn) {
          ingressTransport_.reconnect();
        }
        // Egress socket won't passively notify about disconnect, force reconnect
        egressTransport_.reconnect();
        scheduleConnectionTimer();
      }
    });
  }

  /**
   * @brief Ctor readability helpers
   */
  TraderTcpTransport createIngressTransport() {
    return TraderTcpTransport{
        TcpSocket{ioCtx_}, TcpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portTcpOut},
        BusWrapper{bus_}};
  }

  TraderTcpTransport createEgressTransport() {
    return TraderTcpTransport{TcpSocket{ioCtx_},
                              TcpEndpoint{Ip::make_address(Config::cfg.url), Config::cfg.portTcpIn},
                              BusWrapper{bus_}};
  }

  TraderUdpTransport createPricesTransport() {
    return TraderUdpTransport{utils::createUdpSocket(ioCtx_, false, Config::cfg.portUdp),
                              UdpEndpoint(Udp::v4(), Config::cfg.portUdp), BusWrapper{bus_}};
  }

private:
  IoContext ioCtx_;
  ContextGuard ioCtxGuard_;

  Bus &bus_;

  TraderTcpTransport ingressTransport_;
  TraderTcpTransport egressTransport_;
  TraderUdpTransport pricesTransport_;

  SteadyTimer connectionTimer_;
  Seconds monitorRate_;

  std::atomic_bool connected_{false};
  std::vector<Thread> workerThreads_;
};

} // namespace hft::trader

#endif // HFT_SERVER_NETWORKCLIENT_HPP
