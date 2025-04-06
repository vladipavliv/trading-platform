/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_NETWORKSERVER_HPP
#define HFT_SERVER_NETWORKSERVER_HPP

#include <algorithm>
#include <format>
#include <memory>

#include "boost_types.hpp"
#include "broadcast_service.hpp"
#include "bus/bus.hpp"
#include "config/config.hpp"
#include "gateway.hpp"
#include "market_types.hpp"
#include "network/transport/udp_transport.hpp"
#include "network_types.hpp"
#include "server_command.hpp"
#include "server_events.hpp"
#include "template_types.hpp"
#include "types.hpp"
#include "utils/template_utils.hpp"
#include "utils/utils.hpp"

namespace hft::server {

/**
 * @brief
 */
class NetworkServer {
public:
  NetworkServer(Bus &bus)
      : guard_{MakeGuard(ioCtx_.get_executor())}, bus_{bus}, gateway_{bus_},
        broadcast_{ioCtx_, bus_}, upstreamAcceptor_{ioCtx_}, downstreamAcceptor_{ioCtx_} {}

  ~NetworkServer() { stop(); }

  void start() {
    const auto cores = Config::cfg.coresNetwork.size();
    LOG_INFO_SYSTEM("Starting network server on {} threads", cores);
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
      startIngress();
      startEgress();
      LOG_INFO_SYSTEM("Network server started");
    });
  }

  void stop() { ioCtx_.stop(); }

private:
  void startIngress() {
    TcpEndpoint endpoint(Tcp::v4(), Config::cfg.portTcpIn);
    upstreamAcceptor_.open(endpoint.protocol());
    upstreamAcceptor_.set_option(boost::asio::socket_base::reuse_address(true));
    upstreamAcceptor_.bind(endpoint);
    upstreamAcceptor_.listen();
    acceptUpstream();
  }

  void acceptUpstream() {
    upstreamAcceptor_.async_accept([this](CRef<BoostError> ec, TcpSocket socket) {
      if (ec) {
        LOG_ERROR("Failed to accept connection {}", ec.message());
        return;
      }
      socket.set_option(TcpSocket::protocol_type::no_delay(true));
      gateway_.acceptUpstream(std::move(socket));
      acceptUpstream();
    });
  }

  void startEgress() {
    TcpEndpoint endpoint(Tcp::v4(), Config::cfg.portTcpOut);
    downstreamAcceptor_.open(endpoint.protocol());
    downstreamAcceptor_.set_option(boost::asio::socket_base::reuse_address(true));
    downstreamAcceptor_.bind(endpoint);
    downstreamAcceptor_.listen();
    acceptDownstream();
  }

  void acceptDownstream() {
    downstreamAcceptor_.async_accept([this](CRef<BoostError> ec, TcpSocket socket) {
      if (ec) {
        LOG_ERROR("Failed to accept connection: {}", ec.message());
        return;
      }
      socket.set_option(TcpSocket::protocol_type::no_delay(true));
      gateway_.acceptDownstream(std::move(socket));
      acceptDownstream();
    });
  }

private:
  IoCtx ioCtx_;
  ContextGuard guard_;

  Bus &bus_;
  Gateway gateway_;
  BroadcastService broadcast_;

  TcpAcceptor upstreamAcceptor_;
  TcpAcceptor downstreamAcceptor_;

  std::vector<Thread> workerThreads_;
};

} // namespace hft::server

#endif // HFT_SERVER_NETWORKSERVER_HPP
